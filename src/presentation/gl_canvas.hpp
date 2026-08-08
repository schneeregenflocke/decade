#ifndef GL_CANVAS_HPP
#define GL_CANVAS_HPP

#include <epoxy/gl.h>

#include <QtCore/QString>
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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <glm/vec2.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "../application/calendar/text_input_event.hpp"
#include "../application/render_surface.hpp"
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

// The drawing window: it owns the rendering engine, the view (projection,
// camera) and the pointer input. It knows no domain logic — it receives the
// page size and reports pointer positions in page space onwards.
//
// The deferred context setup that used to need a bootstrap class of its own is
// gone: QOpenGLWidget calls initializeGL() once the context stands and makes it
// current for that call, for resizeGL() and for paintGL(). Outside those three
// no GL function may run without makeCurrent() — the export path is the only
// place that needs it.
class GLCanvas : public QOpenGLWidget, public application::RenderSurface {
 public:
  // Resolution and multisampling of the PNG export (SavePNG). Public, so the
  // menu caption uses the same value instead of a second number in its text.
  static constexpr int kExportPngDpi = 200;
  static constexpr int kExportMsaaSamples = 16;

  // The context version the renderer needs. The format itself gets set once,
  // application-wide, before the QApplication exists (SurfaceFormat below);
  // here it serves as the check that the driver really delivered it.
  static constexpr int kRequiredGlMajor = 4;
  static constexpr int kRequiredGlMinor = 6;

  // The surface format of the whole application. Qt demands it be set before
  // the QApplication is constructed — a per-widget format is honoured on some
  // platforms alone, and the canvas would then quietly get a compatibility
  // context.
  // https://doc.qt.io/qt-6/qopenglwidget.html#details
  [[nodiscard]] static QSurfaceFormat SurfaceFormat() {
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

  explicit GLCanvas(QWidget* parent) : QOpenGLWidget(parent) {
    // Otherwise the surrounding widgets catch Enter, Escape and the arrow keys
    // before the text editor in the canvas sees them.
    setFocusPolicy(Qt::StrongFocus);
    // Hovering has to be reported without a button held down.
    setMouseTracking(true);
  }

  // The engine owns GL objects, and deleting one without a current context is
  // a call into nothing: Qt makes the context current for initializeGL,
  // resizeGL and paintGL alone. The base destructor runs after this one, so the
  // context is still there to be made current.
  ~GLCanvas() override {
    if (graphics_engine_) {
      makeCurrent();
      graphics_engine_.reset();
    }
  }

  GLCanvas(const GLCanvas&) = delete;
  GLCanvas& operator=(const GLCanvas&) = delete;
  GLCanvas(GLCanvas&&) = delete;
  GLCanvas& operator=(GLCanvas&&) = delete;

  // Announces the deferred GL setup. Exactly one of the two callbacks runs; on
  // success the engine stands ready afterwards.
  //
  // When the context stands already the answer follows at once instead of
  // never: which of the two happens is the platform's decision, not ours — some
  // plugins run initializeGL on show(), others at the first paint. The ready
  // callback builds GL objects either way, so it gets a current context here.
  // Inside initializeGL Qt has already seen to that.
  void InitOpenGL(std::function<void()> on_ready,
                  std::function<void(const std::string&)> on_failed) {
    on_ready_ = std::move(on_ready);
    on_failed_ = std::move(on_failed);
    if (graphics_engine_) {
      makeCurrent();
      ReportReady();
    }
  }

  [[nodiscard]] GraphicsEngine& Engine() { return *graphics_engine_; }

  [[nodiscard]] bool HasEngine() const { return graphics_engine_ != nullptr; }

  // Makes this canvas's context current, for building or destroying GL objects
  // outside the three rendering callbacks. Whoever owns GL resources tied to
  // this canvas brackets their work with it — the rendering adapter on every
  // rebuild, the composition root before dissolving the scene.
  //
  // There is deliberately no counterpart: releasing the context again would
  // pull it out from under Qt whenever this runs nested inside a rendering
  // callback, which is exactly where the first scene gets built.
  void MakeGraphicsCurrent() override { makeCurrent(); }

  // Called on every mouse movement with the pointer in page space, so an
  // interaction controller can hit-test on it. The binder sets it.
  void SetPointerMoveCallback(std::function<void(glm::vec2)> callback) {
    on_pointer_move_ = std::move(callback);
  }

  // Click and double click in page space — selection and "please edit".
  void SetPrimaryDownCallback(std::function<void(glm::vec2, bool)> callback) {
    on_primary_down_ = std::move(callback);
  }

  void SetDoubleClickCallback(std::function<void(glm::vec2)> callback) {
    on_double_click_ = std::move(callback);
  }

  // Keyboard input of the running text edit. `editing` says whether an edit is
  // under way — only then does the canvas consume keys, otherwise it passes
  // them on. `selected_text` delivers the selection for the clipboard.
  void SetTextInputCallback(
      std::function<void(const TextInputEvent&)> callback) {
    on_text_input_ = std::move(callback);
  }

  void SetEditingQuery(std::function<bool()> query) {
    is_editing_ = std::move(query);
  }

  void SetSelectedTextSource(std::function<std::string()> source) {
    selected_text_ = std::move(source);
  }

  void ReceivePageSetup(const PageSetupConfig& page_setup_config) {
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

  // Refits projection and zoom bounds to the current window and page size and
  // asks for a repaint. Needed when the page or the canvas size changes. It
  // touches no GL — the viewport is set where the context is current, in
  // paintGL — so it may be called from anywhere.
  void RefreshView() override {
    if (page_size_.Width() <= 0.0F || page_size_.Height() <= 0.0F) {
      if (decade_debug::LogEnabled()) {
        std::cout << "RefreshView: skipped, page_size not yet initialised\n";
      }
      return;
    }
    UpdateProjection();
    update();
  }

  // Triggers a repaint alone — for changes touching neither projection nor zoom
  // bounds (hover and selection colours). Markedly cheaper than RefreshView.
  void Repaint() override { update(); }

  // The frame rate in the one-second window of the newest frame; since drawing
  // happens event-driven alone, the value carries meaning during an
  // interaction.
  [[nodiscard]] double CurrentFps() const { return frame_stats_.Fps(); }

  // The off-screen framebuffers of the export come into being in this context,
  // so it has to be current — this runs outside the rendering callbacks.
  void SavePNG(const std::string& file_path, int dpi = kExportPngDpi) {
    makeCurrent();
    WritePageToPng(file_path, page_size_, static_cast<float>(dpi),
                   *graphics_engine_, kExportMsaaSamples);
  }

  // The rendered canvas content as an image with its origin top left, so it can
  // be mounted into a whole-window screenshot: the widget capture does not see
  // the GL surface. QOpenGLWidget draws into a framebuffer object of its own,
  // which is what makes this readable at all — and readable on every platform,
  // Wayland included.
  [[nodiscard]] QImage CaptureImage() { return AsOpaque(grabFramebuffer()); }

 protected:
  // Runs exactly once, as soon as the context stands. A failure here is final:
  // the shaders exist by now or not at all, so it gets reported rather than
  // retried.
  void initializeGL() override {
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

  void resizeGL(int /*width*/, int /*height*/) override { RefreshView(); }

  void paintGL() override {
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

  void mousePressEvent(QMouseEvent* event) override {
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

  void mouseReleaseEvent(QMouseEvent* event) override {
    HandlePointer(PhysicalPosition(event->position()), false, 0);
  }

  void mouseMoveEvent(QMouseEvent* event) override {
    HandlePointer(PhysicalPosition(event->position()),
                  event->buttons() != Qt::NoButton, 0);
  }

  void wheelEvent(QWheelEvent* event) override {
    HandlePointer(PhysicalPosition(event->position()), false,
                  event->angleDelta().y());
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override {
    setFocus();
    if (on_double_click_) {
      on_double_click_(MouseInteraction::ScreenToPage(
          PhysicalPosition(event->position()), ViewportSize(), mvp_));
    }
  }

  // One handler for the whole keyboard, where wx needed two: Qt delivers the
  // named keys and the composed character in the same event — event->text()
  // stands fully composed for umlauts and accents.
  void keyPressEvent(QKeyEvent* event) override {
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

 private:
  static constexpr char16_t kFirstPrintable = 32;

  struct FramebufferSize {
    GLsizei width{0};
    GLsizei height{0};
  };

  void ReportReady() {
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

  void ReportFailure(const std::string& message) {
    if (on_failed_) {
      const auto failed = std::exchange(on_failed_, nullptr);
      on_ready_ = nullptr;
      failed(message);
      return;
    }
    std::cerr << message << '\n';
  }

  // The format the context really came up with, not the one that was asked
  // for: a driver may hand back less, and the renderer needs to hear about it
  // here rather than at the first shader.
  [[nodiscard]] static bool HasRequiredVersion() {
    const QSurfaceFormat format = QOpenGLContext::currentContext()->format();
    return format.majorVersion() > kRequiredGlMajor ||
           (format.majorVersion() == kRequiredGlMajor &&
            format.minorVersion() >= kRequiredGlMinor);
  }

  [[nodiscard]] static std::string VersionFailureMessage() {
    return "Creating an OpenGL " + std::to_string(kRequiredGlMajor) + "." +
           std::to_string(kRequiredGlMinor) +
           " core context failed. decade needs OpenGL to draw the calendar and "
           "cannot continue.";
  }

  // Retags a read-back as opaque. The canvas draws opaquely, but its alpha
  // channel is not 1 everywhere: blending with GL_SRC_ALPHA leaves a value
  // below it wherever something translucent was drawn, while the colour is
  // already final. A platform whose window surface carries an alpha channel
  // (Wayland does, X11 here does not) hands the image back tagged
  // *premultiplied*, and Qt would then divide those final colours by that
  // leftover alpha on the next draw — teal turns magenta. Retagging costs
  // nothing: the same bytes, read as what they are.
  [[nodiscard]] static QImage AsOpaque(QImage image) {
    switch (image.format()) {
      case QImage::Format_ARGB32:
      case QImage::Format_ARGB32_Premultiplied:
        image.reinterpretAsFormat(QImage::Format_RGB32);
        break;
      case QImage::Format_RGBA8888:
      case QImage::Format_RGBA8888_Premultiplied:
        image.reinterpretAsFormat(QImage::Format_RGBX8888);
        break;
      default:
        break;
    }
    return image;
  }

  [[nodiscard]] static std::string NoContextMessage() {
    return "This platform carries no OpenGL widget. decade needs OpenGL to "
           "draw the calendar and cannot continue.";
  }

  // Valid with a current context alone; without one glGetString returns
  // nullptr, hence the fallback instead of a cast into the void.
  [[nodiscard]] static std::string DescribeDriver() {
    return "GL_VERSION " + DriverString(GL_VERSION) + "\nGL_VENDOR " +
           DriverString(GL_VENDOR) + "\nGL_RENDERER " +
           DriverString(GL_RENDERER);
  }

  [[nodiscard]] static std::string DriverString(GLenum name) {
    const auto* value = glGetString(name);
    if (value == nullptr) {
      return "unknown";
    }
    return {reinterpret_cast<const char*>(value)};
  }

  static void ApplyInitialGlState() {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // The application-wide surface format asks for 4x MSAA, which is why the
    // enable takes hold here.
    glEnable(GL_MULTISAMPLE);

    GLint msaa_samples = 0;
    glGetIntegerv(GL_SAMPLES, &msaa_samples);
    if (decade_debug::LogEnabled()) {
      std::cout << "msaa_samples " << msaa_samples << '\n';
    }
  }

  // The window size in device pixels — more than the logical one on HiDPI
  // displays, and the size of the framebuffer QOpenGLWidget renders into.
  [[nodiscard]] FramebufferSize CurrentFramebufferSize() const {
    const double scale = devicePixelRatioF();
    return {.width = static_cast<GLsizei>(std::lround(width() * scale)),
            .height = static_cast<GLsizei>(std::lround(height() * scale))};
  }

  [[nodiscard]] glm::ivec2 ViewportSize() const {
    const FramebufferSize framebuffer = CurrentFramebufferSize();
    return {framebuffer.width, framebuffer.height};
  }

  void UpdateProjection() {
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

  // Logs FPS and render duration at most once a second (debug mode).
  void LogFrameStats(FrameStats::Clock::time_point now) {
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

  [[nodiscard]] glm::ivec2 PhysicalPosition(const QPointF& position) const {
    const double scale = devicePixelRatioF();
    return {static_cast<int>(std::lround(position.x() * scale)),
            static_cast<int>(std::lround(position.y() * scale))};
  }

  // The one place pointer input turns into camera movement, shared by press,
  // move, release and wheel — they differ in two flags alone.
  void HandlePointer(glm::ivec2 position_physical, bool dragging,
                     int wheel_rotation) {
    const glm::ivec2 viewport = ViewportSize();
    mouse_interaction_.Apply(mvp_, camera_, position_physical, viewport,
                             dragging, wheel_rotation);
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

  // Ctrl-A, C, X and V: the clipboard part of the toolkit stays here, the
  // editor sees reading the selection and inserting text alone.
  bool HandleClipboard(int key) {
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

  void CopySelection() const {
    if (!selected_text_) {
      return;
    }
    const std::string text = selected_text_();
    if (text.empty()) {
      return;
    }
    QGuiApplication::clipboard()->setText(QString::fromStdString(text));
  }

  [[nodiscard]] static std::string ClipboardText() {
    // Line breaks have no business in a single-line caption.
    return SingleLine(QGuiApplication::clipboard()->text().toStdString());
  }

  [[nodiscard]] static std::string SingleLine(std::string text) {
    std::erase(text, '\n');
    std::erase(text, '\r');
    return text;
  }

  [[nodiscard]] bool IsEditing() const { return is_editing_ && is_editing_(); }

  void Send(const TextInputEvent& event) const {
    if (on_text_input_) {
      on_text_input_(event);
    }
  }

  void SendMove(TextEditBuffer::Direction direction,
                TextEditBuffer::Selection selection) const {
    Send(TextInputEvent::Move(direction, selection));
  }

  std::unique_ptr<GraphicsEngine> graphics_engine_;

  std::function<void()> on_ready_;
  std::function<void(const std::string&)> on_failed_;

  std::function<void(glm::vec2, bool)> on_primary_down_;
  std::function<void(glm::vec2)> on_double_click_;
  std::function<void(const TextInputEvent&)> on_text_input_;
  std::function<bool()> is_editing_;
  std::function<std::string()> selected_text_;

  MouseInteraction mouse_interaction_;
  PanZoomCamera camera_;
  RectF page_size_;
  MVP mvp_;

  FrameStats frame_stats_;
  FrameStats::Clock::time_point last_fps_log_;

  std::function<void(glm::vec2)> on_pointer_move_;
};
#endif  // GL_CANVAS_HPP
