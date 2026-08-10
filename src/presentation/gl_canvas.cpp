#include "gl_canvas.hpp"

#include <epoxy/gl.h>

#include <QtCore/QPoint>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QWheelEvent>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtWidgets/QWidget>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "../application/calendar/text_input_event.hpp"
#include "../common/debug_log.hpp"
#include "../domain/page_setup_config.hpp"
#include "../domain/text_edit_buffer.hpp"
#include "../infrastructure/graphics/frame_stats.hpp"
#include "../infrastructure/graphics/graphics_engine.hpp"
#include "../infrastructure/graphics/mvp_matrices.hpp"
#include "../infrastructure/graphics/page_geometry.hpp"
#include "../infrastructure/graphics/pan_zoom_camera.hpp"
#include "../infrastructure/graphics/projection.hpp"
#include "../infrastructure/graphics/rect.hpp"
#include "../infrastructure/graphics/render_to_png.hpp"
#include "mouse_interaction.hpp"

QSurfaceFormat GLCanvas::SurfaceFormat() {
  constexpr int kMsaaSamples = 4;
  constexpr int kDepthBufferBits = 24;
  QSurfaceFormat format;
  format.setVersion(kRequiredGlMajor, kRequiredGlMinor);
  format.setProfile(QSurfaceFormat::CoreProfile);
  // Desktop GL explicitly. On an EGL platform (Wayland) Qt otherwise resolves
  // the default renderable type to OpenGL ES, and the 4.6 core request then
  // fails to match a config — EGL_BAD_MATCH, with no window.
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setDepthBufferSize(kDepthBufferBits);
  format.setSamples(kMsaaSamples);
  return format;
}

GLCanvas::GLCanvas(QWidget* parent) : QOpenGLWidget(parent) {
  // Otherwise the surrounding widgets catch Enter, Escape and the arrow keys
  // before the text editor in the canvas sees them.
  setFocusPolicy(Qt::StrongFocus);
  // Hovering has to be reported without a button held down.
  setMouseTracking(true);
}

GLCanvas::~GLCanvas() {
  if (graphics_engine_) {
    makeCurrent();
    graphics_engine_.reset();
  }
}

void GLCanvas::InitOpenGL(std::function<void()> on_ready,
                          std::function<void(const std::string&)> on_failed) {
  on_ready_ = std::move(on_ready);
  on_failed_ = std::move(on_failed);
  if (graphics_engine_) {
    makeCurrent();
    ReportReady();
  }
}

GraphicsEngine& GLCanvas::Engine() { return *graphics_engine_; }

bool GLCanvas::HasEngine() const { return graphics_engine_ != nullptr; }

void GLCanvas::MakeGraphicsCurrent() { makeCurrent(); }

void GLCanvas::SetPointerMoveCallback(std::function<void(glm::vec2)> callback) {
  on_pointer_move_ = std::move(callback);
}

void GLCanvas::SetPrimaryDownCallback(
    std::function<void(glm::vec2, bool)> callback) {
  on_primary_down_ = std::move(callback);
}

void GLCanvas::SetDoubleClickCallback(std::function<void(glm::vec2)> callback) {
  on_double_click_ = std::move(callback);
}

void GLCanvas::SetTextInputCallback(
    std::function<void(const TextInputEvent&)> callback) {
  on_text_input_ = std::move(callback);
}

void GLCanvas::SetEditingQuery(std::function<bool()> query) {
  is_editing_ = std::move(query);
}

void GLCanvas::SetSelectedTextSource(std::function<std::string()> source) {
  selected_text_ = std::move(source);
}

void GLCanvas::ReceivePageSetup(const PageSetupConfig& page_setup_config) {
  page_size_ = PageRect(page_setup_config);
  if (decade_debug::LogEnabled()) {
    std::cout << "ReceivePageSetup: page=" << page_size_.Width() << "x"
              << page_size_.Height() << " rect=(" << page_size_.Left() << ","
              << page_size_.Right() << "," << page_size_.Bottom() << ","
              << page_size_.Top() << ")\n";
  }
  if (graphics_engine_) {
    RefreshView();
  }
}

void GLCanvas::RefreshView() {
  if (page_size_.Width() <= 0.0F || page_size_.Height() <= 0.0F) {
    if (decade_debug::LogEnabled()) {
      std::cout << "RefreshView: skipped, page_size not yet initialised\n";
    }
    return;
  }
  UpdateProjection();
  update();
}

void GLCanvas::Repaint() { update(); }

double GLCanvas::CurrentFps() const { return frame_stats_.Fps(); }

void GLCanvas::SavePNG(const std::string& file_path, int dpi) {
  makeCurrent();
  WritePageToPng(file_path, page_size_, static_cast<float>(dpi),
                 *graphics_engine_, kExportMsaaSamples);
}

QImage GLCanvas::CaptureImage() { return grabFramebuffer(); }

void GLCanvas::initializeGL() {
  // A platform that carries no QOpenGLWidget (the offscreen plugin, for one)
  // still runs this callback — with no context current. Every GL call would
  // then dispatch through nothing, so the check comes before the first one.
  if (QOpenGLContext::currentContext() == nullptr) {
    ReportFailure(NoContextMessage());
    return;
  }
  if (!HasRequiredVersion()) {
    ReportFailure(VersionFailureMessage());
    return;
  }
  if (decade_debug::LogEnabled()) {
    std::cout << "OpenGL ready, version: " << DescribeDriver() << '\n';
  }

  ApplyInitialGlState();
  try {
    graphics_engine_ = std::make_unique<GraphicsEngine>();
  } catch (const std::exception& error) {
    graphics_engine_.reset();
    ReportFailure(error.what());
    return;
  }
  ReportReady();
}

void GLCanvas::resizeGL(int /*width*/, int /*height*/) { RefreshView(); }

void GLCanvas::paintGL() {
  if (!graphics_engine_) {
    return;
  }
  const auto render_start = FrameStats::Clock::now();
  const FramebufferSize framebuffer = CurrentFramebufferSize();
  glViewport(0, 0, framebuffer.width, framebuffer.height);
  graphics_engine_->SetMVP(mvp_);
  graphics_engine_->Render();
  const auto render_end = FrameStats::Clock::now();
  frame_stats_.AddFrame(render_end, render_end - render_start);
  LogFrameStats(render_end);
}

void GLCanvas::mousePressEvent(QMouseEvent* event) {
  const glm::ivec2 position_physical = PhysicalPosition(event->position());
  if (event->button() == Qt::LeftButton) {
    setFocus();
    if (on_primary_down_) {
      on_primary_down_(MouseInteraction::ScreenToPage(position_physical,
                                                      ViewportSize(), mvp_),
                       event->modifiers().testFlag(Qt::ShiftModifier));
    }
  }
  HandlePointer(position_physical, false, 0);
}

void GLCanvas::mouseReleaseEvent(QMouseEvent* event) {
  HandlePointer(PhysicalPosition(event->position()), false, 0);
}

void GLCanvas::mouseMoveEvent(QMouseEvent* event) {
  HandlePointer(PhysicalPosition(event->position()),
                event->buttons() != Qt::NoButton, 0);
}

void GLCanvas::wheelEvent(QWheelEvent* event) {
  HandlePointer(PhysicalPosition(event->position()), false,
                event->angleDelta().y());
}

void GLCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
  setFocus();
  if (on_double_click_) {
    on_double_click_(MouseInteraction::ScreenToPage(
        PhysicalPosition(event->position()), ViewportSize(), mvp_));
  }
}

void GLCanvas::keyPressEvent(QKeyEvent* event) {
  if (!IsEditing()) {
    QOpenGLWidget::keyPressEvent(event);
    return;
  }
  const auto selection = event->modifiers().testFlag(Qt::ShiftModifier)
                             ? TextEditBuffer::Selection::kExtend
                             : TextEditBuffer::Selection::kReplace;

  if (event->modifiers().testFlag(Qt::ControlModifier) &&
      HandleClipboard(event->key())) {
    return;
  }

  switch (event->key()) {
    case Qt::Key_Left:
      SendMove(TextEditBuffer::Direction::kLeft, selection);
      return;
    case Qt::Key_Right:
      SendMove(TextEditBuffer::Direction::kRight, selection);
      return;
    case Qt::Key_Home:
      SendMove(TextEditBuffer::Direction::kBegin, selection);
      return;
    case Qt::Key_End:
      SendMove(TextEditBuffer::Direction::kEnd, selection);
      return;
    case Qt::Key_Backspace:
      Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteBefore));
      return;
    case Qt::Key_Delete:
      Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteAfter));
      return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      Send(TextInputEvent::Command(TextInputEvent::Kind::kCommit));
      return;
    case Qt::Key_Escape:
      Send(TextInputEvent::Command(TextInputEvent::Kind::kCancel));
      return;
    default:
      break;
  }

  const QString text = event->text();
  if (text.isEmpty() || text.at(0).unicode() < kFirstPrintable) {
    QOpenGLWidget::keyPressEvent(event);
    return;
  }
  Send(TextInputEvent::Insert(text.toStdString()));
}

void GLCanvas::ReportReady() {
  // The consumer builds GL objects, so it may only be called with a context
  // current. A platform that carries no OpenGL widget leaves none behind even
  // after makeCurrent(); reporting that here turns an abort in the first
  // buffer allocation into the failure path the window already knows.
  if (QOpenGLContext::currentContext() == nullptr) {
    ReportFailure(NoContextMessage());
    return;
  }
  if (on_ready_) {
    // Moved out first: the callback may destroy what holds this canvas, and a
    // member still being read after that would be a use-after-free.
    const auto ready = std::exchange(on_ready_, nullptr);
    on_failed_ = nullptr;
    ready();
  }
}

void GLCanvas::ReportFailure(const std::string& message) {
  if (on_failed_) {
    const auto failed = std::exchange(on_failed_, nullptr);
    on_ready_ = nullptr;
    failed(message);
    return;
  }
  std::cerr << message << '\n';
}

bool GLCanvas::HasRequiredVersion() {
  const QSurfaceFormat format = QOpenGLContext::currentContext()->format();
  return format.majorVersion() > kRequiredGlMajor ||
         (format.majorVersion() == kRequiredGlMajor &&
          format.minorVersion() >= kRequiredGlMinor);
}

std::string GLCanvas::VersionFailureMessage() {
  return "Creating an OpenGL " + std::to_string(kRequiredGlMajor) + "." +
         std::to_string(kRequiredGlMinor) +
         " core context failed. decade needs OpenGL to draw the calendar and "
         "cannot continue.";
}

std::string GLCanvas::NoContextMessage() {
  return "This platform carries no OpenGL widget. decade needs OpenGL to "
         "draw the calendar and cannot continue.";
}

std::string GLCanvas::DescribeDriver() {
  return "GL_VERSION " + DriverString(GL_VERSION) + "\nGL_VENDOR " +
         DriverString(GL_VENDOR) + "\nGL_RENDERER " + DriverString(GL_RENDERER);
}

std::string GLCanvas::DriverString(GLenum name) {
  const auto* value = glGetString(name);
  if (value == nullptr) {
    return "unknown";
  }
  return {reinterpret_cast<const char*>(value)};
}

void GLCanvas::ApplyInitialGlState() {
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  // Colour blends the usual way; alpha saturates instead of blending with
  // itself. GL_SRC_ALPHA on the alpha channel too would leave a value below
  // 1 wherever something translucent was drawn (0.35 over 1.0 gives 0.77),
  // and the canvas draws an opaque page — that leftover is meaningless.
  // Whoever reads the buffer back as premultiplied divides the final colours
  // by it and wraps at 8 bit: teal turns red, purple turns yellow.
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                      GL_ONE_MINUS_SRC_ALPHA);
  // The application-wide surface format asks for 4x MSAA, which is why the
  // enable takes hold here.
  glEnable(GL_MULTISAMPLE);

  GLint msaa_samples = 0;
  glGetIntegerv(GL_SAMPLES, &msaa_samples);
  if (decade_debug::LogEnabled()) {
    std::cout << "msaa_samples " << msaa_samples << '\n';
  }
}

GLCanvas::FramebufferSize GLCanvas::CurrentFramebufferSize() const {
  const double scale = devicePixelRatioF();
  return {.width = static_cast<GLsizei>(std::lround(width() * scale)),
          .height = static_cast<GLsizei>(std::lround(height() * scale))};
}

glm::ivec2 GLCanvas::ViewportSize() const {
  const FramebufferSize framebuffer = CurrentFramebufferSize();
  return {framebuffer.width, framebuffer.height};
}

void GLCanvas::UpdateProjection() {
  constexpr float kViewSizeScale = 1.1F;
  const RectF view_size = page_size_.Scale(kViewSizeScale);
  const glm::ivec2 viewport = ViewportSize();

  mvp_.SetProjection(Projection::OrthoMatrix(
      view_size, Projection::AspectRatioOf(viewport.x, viewport.y)));
  camera_.SetScaleLimits(ComputeZoomLimits(
      mvp_.GetProjection(), {page_size_.Width(), page_size_.Height()},
      static_cast<float>(kExportPngDpi)));

  if (decade_debug::LogEnabled()) {
    std::cout << "UpdateProjection: page=" << page_size_.Width() << "x"
              << page_size_.Height() << " view=" << view_size.Width() << "x"
              << view_size.Height() << '\n';
    decade_debug::LogMat4("UpdateProjection proj", mvp_.GetProjection());
    decade_debug::LogMat4("UpdateProjection view", mvp_.GetView());
  }
}

void GLCanvas::LogFrameStats(FrameStats::Clock::time_point now) {
  if (!decade_debug::LogEnabled()) {
    return;
  }
  if (now - last_fps_log_ < std::chrono::seconds(1)) {
    return;
  }
  last_fps_log_ = now;
  std::cout << "FPS: " << frame_stats_.Fps() << " (render "
            << frame_stats_.LastRenderMillis() << " ms)\n";
}

glm::ivec2 GLCanvas::PhysicalPosition(const QPointF& position) const {
  const double scale = devicePixelRatioF();
  return {static_cast<int>(std::lround(position.x() * scale)),
          static_cast<int>(std::lround(position.y() * scale))};
}

void GLCanvas::HandlePointer(glm::ivec2 position_physical, bool dragging,
                             int wheel_rotation) {
  const glm::ivec2 viewport = ViewportSize();
  mouse_interaction_.Apply(mvp_, camera_, position_physical, viewport, dragging,
                           wheel_rotation);
  // Report the pointer in page space onwards, after Apply, so the view just
  // panned or zoomed is the one that counts.
  if (on_pointer_move_) {
    on_pointer_move_(
        MouseInteraction::ScreenToPage(position_physical, viewport, mvp_));
  }
  // Dragging and the mouse wheel alone change the view; a bare pointer
  // movement triggers no repaint — a hover change triggers its own through
  // CalendarPage::ReceiveHovered. Projection and zoom bounds stay untouched,
  // so RefreshView is not needed here.
  if (dragging || wheel_rotation != 0) {
    Repaint();
  }
}

bool GLCanvas::HandleClipboard(int key) {
  switch (key) {
    case Qt::Key_A:
      Send(TextInputEvent::Command(TextInputEvent::Kind::kSelectAll));
      return true;
    case Qt::Key_C:
      CopySelection();
      return true;
    case Qt::Key_X:
      CopySelection();
      Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteBefore));
      return true;
    case Qt::Key_V:
      Send(TextInputEvent::Insert(ClipboardText()));
      return true;
    default:
      return false;
  }
}

void GLCanvas::CopySelection() const {
  if (!selected_text_) {
    return;
  }
  const std::string text = selected_text_();
  if (text.empty()) {
    return;
  }
  QGuiApplication::clipboard()->setText(QString::fromStdString(text));
}

std::string GLCanvas::ClipboardText() {
  // Line breaks have no business in a single-line caption.
  return SingleLine(QGuiApplication::clipboard()->text().toStdString());
}

std::string GLCanvas::SingleLine(std::string text) {
  std::erase(text, '\n');
  std::erase(text, '\r');
  return text;
}

bool GLCanvas::IsEditing() const { return is_editing_ && is_editing_(); }

void GLCanvas::Send(const TextInputEvent& event) const {
  if (on_text_input_) {
    on_text_input_(event);
  }
}

void GLCanvas::SendMove(TextEditBuffer::Direction direction,
                        TextEditBuffer::Selection selection) const {
  Send(TextInputEvent::Move(direction, selection));
}
