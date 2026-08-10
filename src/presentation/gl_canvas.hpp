#ifndef GL_CANVAS_HPP
#define GL_CANVAS_HPP

// libepoxy has to be first. epoxy/gl.h claims the include guards __gl_h_ and
// __glext_h_ and refuses to compile once GL/gl.h got there before it (an
// #error). Qt's qopengl.h — which QOpenGLWidget pulls in — includes exactly
// those, so a header that reaches Qt first breaks the build.
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
  [[nodiscard]] static QSurfaceFormat SurfaceFormat();

  explicit GLCanvas(QWidget* parent);

  // The engine owns GL objects, and deleting one without a current context is
  // a call into nothing: Qt makes the context current for initializeGL,
  // resizeGL and paintGL alone. The base destructor runs after this one, so the
  // context is still there to be made current.
  ~GLCanvas() override;

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
                  std::function<void(const std::string&)> on_failed);

  [[nodiscard]] GraphicsEngine& Engine();

  [[nodiscard]] bool HasEngine() const;

  // Makes this canvas's context current, for building or destroying GL objects
  // outside the three rendering callbacks. Whoever owns GL resources tied to
  // this canvas brackets their work with it — the rendering adapter on every
  // rebuild, the composition root before dissolving the scene.
  //
  // There is deliberately no counterpart: releasing the context again would
  // pull it out from under Qt whenever this runs nested inside a rendering
  // callback, which is exactly where the first scene gets built.
  void MakeGraphicsCurrent() override;

  // Called on every mouse movement with the pointer in page space, so an
  // interaction controller can hit-test on it. The binder sets it.
  void SetPointerMoveCallback(std::function<void(glm::vec2)> callback);

  // Click and double click in page space — selection and "please edit".
  void SetPrimaryDownCallback(std::function<void(glm::vec2, bool)> callback);

  void SetDoubleClickCallback(std::function<void(glm::vec2)> callback);

  // Keyboard input of the running text edit. `editing` says whether an edit is
  // under way — only then does the canvas consume keys, otherwise it passes
  // them on. `selected_text` delivers the selection for the clipboard.
  void SetTextInputCallback(
      std::function<void(const TextInputEvent&)> callback);

  void SetEditingQuery(std::function<bool()> query);

  void SetSelectedTextSource(std::function<std::string()> source);

  void ReceivePageSetup(const PageSetupConfig& page_setup_config);

  // Refits projection and zoom bounds to the current window and page size and
  // asks for a repaint. Needed when the page or the canvas size changes. It
  // touches no GL — the viewport is set where the context is current, in
  // paintGL — so it may be called from anywhere.
  void RefreshView() override;

  // Triggers a repaint alone — for changes touching neither projection nor zoom
  // bounds (hover and selection colours). Markedly cheaper than RefreshView.
  void Repaint() override;

  // The frame rate in the one-second window of the newest frame; since drawing
  // happens event-driven alone, the value carries meaning during an
  // interaction.
  [[nodiscard]] double CurrentFps() const;

  // The off-screen framebuffers of the export come into being in this context,
  // so it has to be current — this runs outside the rendering callbacks.
  void SavePNG(const std::string& file_path, int dpi = kExportPngDpi);

  // The rendered canvas content as an image with its origin top left, so it can
  // be mounted into a whole-window screenshot: the widget capture does not see
  // the GL surface. QOpenGLWidget draws into a framebuffer object of its own,
  // which is what makes this readable at all — and readable on every platform,
  // Wayland included.
  [[nodiscard]] QImage CaptureImage();

 protected:
  // Runs exactly once, as soon as the context stands. A failure here is final:
  // the shaders exist by now or not at all, so it gets reported rather than
  // retried.
  void initializeGL() override;

  void resizeGL(int /*width*/, int /*height*/) override;

  void paintGL() override;

  void mousePressEvent(QMouseEvent* event) override;

  void mouseReleaseEvent(QMouseEvent* event) override;

  void mouseMoveEvent(QMouseEvent* event) override;

  void wheelEvent(QWheelEvent* event) override;

  void mouseDoubleClickEvent(QMouseEvent* event) override;

  // One handler for the whole keyboard, where wx needed two: Qt delivers the
  // named keys and the composed character in the same event — event->text()
  // stands fully composed for umlauts and accents.
  void keyPressEvent(QKeyEvent* event) override;

 private:
  static constexpr char16_t kFirstPrintable = 32;

  struct FramebufferSize {
    GLsizei width{0};
    GLsizei height{0};
  };

  void ReportReady();

  void ReportFailure(const std::string& message);

  // The format the context really came up with, not the one that was asked
  // for: a driver may hand back less, and the renderer needs to hear about it
  // here rather than at the first shader.
  [[nodiscard]] static bool HasRequiredVersion();

  [[nodiscard]] static std::string VersionFailureMessage();

  [[nodiscard]] static std::string NoContextMessage();

  // Valid with a current context alone; without one glGetString returns
  // nullptr, hence the fallback instead of a cast into the void.
  [[nodiscard]] static std::string DescribeDriver();

  [[nodiscard]] static std::string DriverString(GLenum name);

  static void ApplyInitialGlState();

  // The window size in device pixels — more than the logical one on HiDPI
  // displays, and the size of the framebuffer QOpenGLWidget renders into.
  [[nodiscard]] FramebufferSize CurrentFramebufferSize() const;

  [[nodiscard]] glm::ivec2 ViewportSize() const;

  void UpdateProjection();

  // Logs FPS and render duration at most once a second (debug mode).
  void LogFrameStats(FrameStats::Clock::time_point now);

  [[nodiscard]] glm::ivec2 PhysicalPosition(const QPointF& position) const;

  // The one place pointer input turns into camera movement, shared by press,
  // move, release and wheel — they differ in two flags alone.
  void HandlePointer(glm::ivec2 position_physical, bool dragging,
                     int wheel_rotation);

  // Ctrl-A, C, X and V: the clipboard part of the toolkit stays here, the
  // editor sees reading the selection and inserting text alone.
  bool HandleClipboard(int key);

  void CopySelection() const;

  [[nodiscard]] static std::string ClipboardText();

  [[nodiscard]] static std::string SingleLine(std::string text);

  [[nodiscard]] bool IsEditing() const;

  void Send(const TextInputEvent& event) const;

  void SendMove(TextEditBuffer::Direction direction,
                TextEditBuffer::Selection selection) const;

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
