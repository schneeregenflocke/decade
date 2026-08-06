#ifndef GL_CANVAS_HPP
#define GL_CANVAS_HPP

#include <epoxy/gl.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dcclient.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/glcanvas.h>
#include <wx/image.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <glm/vec2.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
#include "gl_context_bootstrap.hpp"
#include "mouse_interaction.hpp"
#include "wx_owned.hpp"

// The drawing window: it owns the context (through the bootstrap), the
// rendering engine, the view (projection, camera) and the pointer input. It
// knows no domain logic — it receives the page size and reports pointer
// positions in page space onwards.
class GLCanvas : public wxGLCanvas {
 public:
  // Resolution and multisampling of the PNG export (SavePNG). Public, so the
  // menu caption uses the same value instead of a second number in its text.
  static constexpr int kExportPngDpi = 200;
  static constexpr int kExportMsaaSamples = 16;

  explicit GLCanvas(wxWindow* parent)
      // wxWANTS_CHARS: otherwise the dialogue catches Enter, Esc and the arrow
      // keys before the text editor in the canvas sees them.
      : wxGLCanvas(parent, DisplayAttributes(), wxID_ANY, wxDefaultPosition,
                   wxDefaultSize, wxWANTS_CHARS),
        context_bootstrap_(*this, kRequiredGlVersion) {
    if (decade_debug::LogEnabled()) {
      std::cout << "wxGLCanvas IsDisplaySupported " << std::boolalpha
                << wxGLCanvas::IsDisplaySupported(DisplayAttributes()) << '\n';
    }
  }

  // Starts the deferred GL setup. Exactly one of the two callbacks runs; on
  // success the engine stands ready afterwards.
  void InitOpenGL(std::function<void()> on_ready,
                  std::function<void(const std::string&)> on_failed) {
    context_bootstrap_.Start(
        [this, ready = std::move(on_ready)]() {
          StartRendering();
          ready();
        },
        std::move(on_failed));
  }

  [[nodiscard]] GraphicsEngine& Engine() { return *graphics_engine_; }

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
      std::cout << "ReceivePageSetup: page=" << page_size_.width() << "x"
                << page_size_.height() << " rect=(" << page_size_.l() << ","
                << page_size_.r() << "," << page_size_.b() << ","
                << page_size_.t() << ")\n";
    }
    if (graphics_engine_) {
      RefreshView();
    }
  }

  // Fits viewport, projection and zoom bounds to the current window and page
  // size and asks for a repaint. Needed when the page or the canvas size
  // changes.
  void RefreshView() {
    UpdateViewport();
    if (page_size_.width() <= 0.0F || page_size_.height() <= 0.0F) {
      if (decade_debug::LogEnabled()) {
        std::cout << "RefreshView: skipped, page_size not yet initialised\n";
      }
      return;
    }
    UpdateProjection();
    Refresh(false);
  }

  // Triggers a repaint alone — for changes touching neither projection nor zoom
  // bounds (hover and selection colours). Markedly cheaper than RefreshView.
  void Repaint() { Refresh(false); }

  // The frame rate in the one-second window of the newest frame; since drawing
  // happens event-driven alone, the value carries meaning during an
  // interaction.
  [[nodiscard]] double CurrentFps() const { return frame_stats_.Fps(); }

  void SavePNG(const std::string& file_path, int dpi = kExportPngDpi) {
    WritePageToPng(file_path, page_size_, static_cast<float>(dpi),
                   *graphics_engine_, kExportMsaaSamples);
  }

  // Returns the current GL back buffer as an RGB wxImage with its origin top
  // left, so it can be mounted into a whole-window screenshot: a wxDC does not
  // see the GL surface. Without a surface an invalid image comes back.
  wxImage CaptureBackBufferImage() {
    const BackBuffer back = ReadBackBuffer();
    if (back.pixels.empty()) {
      return {};
    }
    wxImage image(static_cast<int>(back.width), static_cast<int>(back.height));
    std::copy_n(back.pixels.data(), back.pixels.size(), image.GetData());
    return image;
  }

 private:
  static constexpr GlContextBootstrap::Version kRequiredGlVersion{.major = 4,
                                                                  .minor = 6};

  struct BackBuffer {
    std::vector<unsigned char> pixels;  // origin top left, rows flipped
    std::size_t width{0};
    std::size_t height{0};
  };

  struct FramebufferSize {
    GLsizei width{0};
    GLsizei height{0};
  };

  // Runs exactly once, as soon as the context stands: set the GL base state,
  // build the engine, attach the draw and input events.
  void StartRendering() {
    ApplyInitialGlState();
    graphics_engine_ = std::make_unique<GraphicsEngine>();

    Bind(wxEVT_SIZE, &GLCanvas::SizeCallback, this);
    Bind(wxEVT_PAINT, &GLCanvas::PaintCallback, this);
    Bind(wxEVT_MOTION, &GLCanvas::MouseCallback, this);
    Bind(wxEVT_LEFT_DOWN, &GLCanvas::MouseCallback, this);
    Bind(wxEVT_LEFT_UP, &GLCanvas::MouseCallback, this);
    Bind(wxEVT_MOUSEWHEEL, &GLCanvas::MouseCallback, this);
    Bind(wxEVT_LEFT_DCLICK, &GLCanvas::DoubleClickCallback, this);
    Bind(wxEVT_KEY_DOWN, &GLCanvas::KeyDownCallback, this);
    Bind(wxEVT_CHAR, &GLCanvas::CharCallback, this);
  }

  static void ApplyInitialGlState() {
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // wxGLAttributes::Defaults() asks for 4x MSAA under GLX
    // (SampleBuffers(1).Samplers(4)), which is why the enable takes hold here.
    glEnable(GL_MULTISAMPLE);

    GLint msaa_samples = 0;
    glGetIntegerv(GL_SAMPLES, &msaa_samples);
    if (decade_debug::LogEnabled()) {
      std::cout << "msaa_samples " << msaa_samples << '\n';
    }
  }

  static wxGLAttributes DisplayAttributes() {
    wxGLAttributes attributes;
    attributes.PlatformDefaults().Defaults().EndList();
    return attributes;
  }

  // The window size in device pixels — more than the logical one on HiDPI
  // displays.
  [[nodiscard]] FramebufferSize CurrentFramebufferSize() const {
    const wxSize logical_size = GetClientSize();
    const double scale = GetContentScaleFactor();
    return {.width = static_cast<GLsizei>(
                std::lround(logical_size.GetWidth() * scale)),
            .height = static_cast<GLsizei>(
                std::lround(logical_size.GetHeight() * scale))};
  }

  void UpdateViewport() const {
    const FramebufferSize framebuffer = CurrentFramebufferSize();
    glViewport(0, 0, framebuffer.width, framebuffer.height);

    if (decade_debug::LogEnabled()) {
      std::cout << "UpdateViewport scale=" << GetContentScaleFactor()
                << " fb=" << framebuffer.width << "x" << framebuffer.height
                << '\n';
    }
  }

  void UpdateProjection() {
    constexpr float kViewSizeScale = 1.1F;
    const rectf view_size = page_size_.scale(kViewSizeScale);

    mvp_.SetProjection(Projection::OrthoMatrix(view_size));
    camera_.SetScaleLimits(ComputeZoomLimits(
        mvp_.GetProjection(), {page_size_.width(), page_size_.height()},
        static_cast<float>(kExportPngDpi)));
    graphics_engine_->SetMVP(mvp_);

    if (decade_debug::LogEnabled()) {
      std::cout << "UpdateProjection: page=" << page_size_.width() << "x"
                << page_size_.height() << " view=" << view_size.width() << "x"
                << view_size.height() << '\n';
      decade_debug::LogMat4("UpdateProjection proj", mvp_.GetProjection());
      decade_debug::LogMat4("UpdateProjection view", mvp_.GetView());
    }
  }

  // Draws the scene into the back buffer and reads it back as an RGB buffer
  // with its origin top left. OpenGL counts from below, hence the row flip.
  // Without a surface an empty buffer comes back.
  BackBuffer ReadBackBuffer() {
    constexpr std::size_t kBytesPerPixel = 3;
    context_bootstrap_.MakeCurrent();
    graphics_engine_->SetMVP(mvp_);
    graphics_engine_->Render();
    glFinish();

    const FramebufferSize framebuffer = CurrentFramebufferSize();
    const auto width = static_cast<std::size_t>(framebuffer.width);
    const auto height = static_cast<std::size_t>(framebuffer.height);
    if (width == 0 || height == 0) {
      return {};
    }
    std::vector<unsigned char> buffer(width * height * kBytesPerPixel);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, framebuffer.width, framebuffer.height, GL_RGB,
                 GL_UNSIGNED_BYTE, buffer.data());

    std::vector<unsigned char> flipped(buffer.size());
    const std::size_t row_bytes = width * kBytesPerPixel;
    for (std::size_t row = 0; row < height; ++row) {
      std::copy_n(buffer.data() + ((height - 1 - row) * row_bytes), row_bytes,
                  flipped.data() + (row * row_bytes));
    }
    return {.pixels = std::move(flipped), .width = width, .height = height};
  }

  void PaintCallback(wxPaintEvent& /*event*/) {
    const wxPaintDC paint_dc(this);
    const auto render_start = FrameStats::Clock::now();
    context_bootstrap_.MakeCurrent();
    graphics_engine_->SetMVP(mvp_);
    graphics_engine_->Render();
    SwapBuffers();
    const auto render_end = FrameStats::Clock::now();
    frame_stats_.AddFrame(render_end, render_end - render_start);
    LogFrameStats(render_end);
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

  void SizeCallback(wxSizeEvent& /*event*/) { RefreshView(); }

  // The pointer position of the event in page space — the unit the scene
  // computes in.
  [[nodiscard]] glm::vec2 PagePoint(const wxMouseEvent& event) const {
    return MouseInteraction::ScreenToPage(PhysicalPosition(event), mvp_);
  }

  [[nodiscard]] wxPoint PhysicalPosition(const wxMouseEvent& event) const {
    const double scale = GetContentScaleFactor();
    return {static_cast<int>(std::lround(event.GetPosition().x * scale)),
            static_cast<int>(std::lround(event.GetPosition().y * scale))};
  }

  void DoubleClickCallback(wxMouseEvent& event) {
    SetFocus();
    if (on_double_click_) {
      on_double_click_(PagePoint(event));
    }
    event.Skip();
  }

  // Translates control keys into their meaning. Printable characters arrive as
  // wxEVT_CHAR alone — there umlauts and accents stand fully composed.
  void KeyDownCallback(wxKeyEvent& event) {
    if (!IsEditing()) {
      event.Skip();
      return;
    }
    const auto selection = event.ShiftDown()
                               ? TextEditBuffer::Selection::kExtend
                               : TextEditBuffer::Selection::kReplace;

    if (event.ControlDown() && HandleClipboard(event)) {
      return;
    }

    switch (event.GetKeyCode()) {
      case WXK_LEFT:
        SendMove(TextEditBuffer::Direction::kLeft, selection);
        return;
      case WXK_RIGHT:
        SendMove(TextEditBuffer::Direction::kRight, selection);
        return;
      case WXK_HOME:
        SendMove(TextEditBuffer::Direction::kBegin, selection);
        return;
      case WXK_END:
        SendMove(TextEditBuffer::Direction::kEnd, selection);
        return;
      case WXK_BACK:
        Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteBefore));
        return;
      case WXK_DELETE:
        Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteAfter));
        return;
      case WXK_RETURN:
      case WXK_NUMPAD_ENTER:
        Send(TextInputEvent::Command(TextInputEvent::Kind::kCommit));
        return;
      case WXK_ESCAPE:
        Send(TextInputEvent::Command(TextInputEvent::Kind::kCancel));
        return;
      default:
        event.Skip();
    }
  }

  void CharCallback(wxKeyEvent& event) {
    constexpr int kFirstPrintable = 32;
    const wxChar character = event.GetUnicodeKey();
    if (!IsEditing() || character == WXK_NONE || character < kFirstPrintable) {
      event.Skip();
      return;
    }
    Send(TextInputEvent::Insert(wxString(character).ToStdString(wxConvUTF8)));
  }

  // Ctrl-C, X and V: the wx part of the clipboard stays here, the editor sees
  // reading the selection and inserting text alone.
  bool HandleClipboard(const wxKeyEvent& event) {
    switch (event.GetKeyCode()) {
      case 'A':
        Send(TextInputEvent::Command(TextInputEvent::Kind::kSelectAll));
        return true;
      case 'C':
        CopySelection();
        return true;
      case 'X':
        CopySelection();
        Send(TextInputEvent::Command(TextInputEvent::Kind::kDeleteBefore));
        return true;
      case 'V':
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
    if (text.empty() || !wxTheClipboard->Open()) {
      return;
    }
    wxTheClipboard->SetData(
        MakeOwned<wxTextDataObject>(wxString::FromUTF8(text)));
    wxTheClipboard->Close();
  }

  [[nodiscard]] static std::string ClipboardText() {
    if (!wxTheClipboard->Open()) {
      return {};
    }
    wxTextDataObject data;
    const bool available = wxTheClipboard->IsSupported(wxDF_UNICODETEXT) &&
                           wxTheClipboard->GetData(data);
    wxTheClipboard->Close();
    // Line breaks have no business in a single-line caption.
    return available ? SingleLine(data.GetText().ToStdString(wxConvUTF8))
                     : std::string{};
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

  void MouseCallback(wxMouseEvent& event) {
    const wxPoint position_physical = PhysicalPosition(event);
    if (event.LeftDown()) {
      SetFocus();
      if (on_primary_down_) {
        on_primary_down_(
            MouseInteraction::ScreenToPage(position_physical, mvp_),
            event.ShiftDown());
      }
    }
    mouse_interaction_.Apply(mvp_, camera_, position_physical, event.Dragging(),
                             event.GetWheelRotation());
    // Report the pointer in page space onwards, after Apply, so the view just
    // panned or zoomed is the one that counts.
    if (on_pointer_move_) {
      on_pointer_move_(MouseInteraction::ScreenToPage(position_physical, mvp_));
    }
    // Dragging and the mouse wheel alone change the view; a bare pointer
    // movement triggers no repaint — a hover change triggers its own through
    // CalendarPage::ReceiveHovered. Projection and zoom bounds stay untouched,
    // so RefreshView is not needed here.
    if (event.Dragging() || event.GetWheelRotation() != 0) {
      Repaint();
    }
  }

  GlContextBootstrap context_bootstrap_;
  std::unique_ptr<GraphicsEngine> graphics_engine_;

  std::function<void(glm::vec2, bool)> on_primary_down_;
  std::function<void(glm::vec2)> on_double_click_;
  std::function<void(const TextInputEvent&)> on_text_input_;
  std::function<bool()> is_editing_;
  std::function<std::string()> selected_text_;

  MouseInteraction mouse_interaction_;
  PanZoomCamera camera_;
  rectf page_size_;
  MVP mvp_;

  FrameStats frame_stats_;
  FrameStats::Clock::time_point last_fps_log_;

  std::function<void(glm::vec2)> on_pointer_move_;
};
#endif  // GL_CANVAS_HPP
