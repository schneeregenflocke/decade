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

// Das Zeichenfenster: besitzt Kontext (über den Bootstrap), Rendering-Engine,
// Ansicht (Projektion, Kamera) und die Zeigereingabe. Es kennt keine
// Domänenlogik — es empfängt die Seitengrösse und meldet Zeigerpositionen im
// Seitenraum weiter.
class GLCanvas : public wxGLCanvas {
 public:
  // Auflösung und Multisampling des PNG-Exports (SavePNG). Öffentlich, damit
  // die Menübeschriftung denselben Wert nutzt statt einer zweiten Zahl im Text.
  static constexpr int kExportPngDpi = 200;
  static constexpr int kExportMsaaSamples = 16;

  explicit GLCanvas(wxWindow* parent)
      // wxWANTS_CHARS: sonst fängt der Dialog Enter, Esc und die Pfeiltasten
      // ab, bevor der Texteditor im Canvas sie sieht.
      : wxGLCanvas(parent, DisplayAttributes(), wxID_ANY, wxDefaultPosition,
                   wxDefaultSize, wxWANTS_CHARS),
        context_bootstrap_(*this, kRequiredGlVersion) {
    std::cout << "wxGLCanvas IsDisplaySupported " << std::boolalpha
              << wxGLCanvas::IsDisplaySupported(DisplayAttributes()) << '\n';
  }

  // Startet den verzögerten GL-Aufbau. Genau einer der beiden Callbacks läuft;
  // im Erfolgsfall steht danach die Engine bereit.
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

  // Wird bei jeder Mausbewegung mit dem Zeiger im Seitenraum aufgerufen, damit
  // ein Interaktions-Controller darauf hit-testen kann. Setzt der Binder.
  void SetPointerMoveCallback(std::function<void(glm::vec2)> callback) {
    on_pointer_move_ = std::move(callback);
  }

  // Klick und Doppelklick im Seitenraum — Auswahl und «bitte bearbeiten».
  void SetPrimaryDownCallback(std::function<void(glm::vec2, bool)> callback) {
    on_primary_down_ = std::move(callback);
  }

  void SetDoubleClickCallback(std::function<void(glm::vec2)> callback) {
    on_double_click_ = std::move(callback);
  }

  // Tastatureingaben der laufenden Textbearbeitung. `editing` sagt, ob gerade
  // bearbeitet wird — nur dann verbraucht das Canvas Tasten, sonst gibt es sie
  // weiter. `selected_text` liefert die Auswahl für die Zwischenablage.
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

  // Passt Viewport, Projektion und Zoom-Grenzen an die aktuelle Fenster- und
  // Seitengrösse an und fordert einen Repaint an. Nötig, wenn sich Seite oder
  // Canvasgrösse ändern.
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

  // Stösst nur einen Repaint an — für Änderungen, die weder Projektion noch
  // Zoom-Grenzen berühren (Hover-/Selektionsfarben). Deutlich billiger als
  // RefreshView.
  void Repaint() { Refresh(false); }

  // Bildrate im Sekundenfenster des jüngsten Frames; da nur ereignisgesteuert
  // gezeichnet wird, ist der Wert während einer Interaktion aussagekräftig.
  [[nodiscard]] double CurrentFps() const { return frame_stats_.Fps(); }

  void SavePNG(const std::string& file_path, int dpi = kExportPngDpi) {
    WritePageToPng(file_path, page_size_, static_cast<float>(dpi),
                   *graphics_engine_, kExportMsaaSamples);
  }

  // Gibt den aktuellen GL-Backbuffer als RGB-wxImage mit Ursprung links oben
  // zurück, damit er in einen Gesamtfenster-Screenshot montiert werden kann:
  // ein wxDC sieht die GL-Fläche nicht. Ohne Fläche kommt ein ungültiges Bild.
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
    std::vector<unsigned char> pixels;  // Ursprung links oben, Zeilen gedreht
    std::size_t width{0};
    std::size_t height{0};
  };

  struct FramebufferSize {
    GLsizei width{0};
    GLsizei height{0};
  };

  // Läuft genau einmal, sobald der Kontext steht: GL-Grundzustand setzen,
  // Engine bauen, Zeichen- und Eingabeereignisse anhängen.
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
    // wxGLAttributes::Defaults() fordert unter GLX 4x MSAA an
    // (SampleBuffers(1).Samplers(4)), deshalb greift das Enable hier.
    glEnable(GL_MULTISAMPLE);

    GLint msaa_samples = 0;
    glGetIntegerv(GL_SAMPLES, &msaa_samples);
    std::cout << "msaa_samples " << msaa_samples << '\n';
  }

  static wxGLAttributes DisplayAttributes() {
    wxGLAttributes attributes;
    attributes.PlatformDefaults().Defaults().EndList();
    return attributes;
  }

  // Fenstergrösse in Gerätepixeln — auf HiDPI-Anzeigen mehr als die logische.
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

  // Zeichnet die Szene in den Backbuffer und liest ihn als RGB-Puffer mit
  // Ursprung links oben zurück. OpenGL zählt von unten, deshalb das Drehen der
  // Zeilen. Ohne Fläche kommt ein leerer Puffer.
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

  // Loggt FPS und Renderdauer höchstens einmal pro Sekunde (Debug-Modus).
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

  // Zeigerposition des Ereignisses im Seitenraum — die Einheit, in der die
  // Szene rechnet.
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

  // Übersetzt Steuertasten in ihre Bedeutung. Druckbare Zeichen kommen erst als
  // wxEVT_CHAR an — dort stehen Umlaute und Akzente fertig zusammengesetzt.
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

  // Ctrl-C/X/V: der wx-Teil der Zwischenablage bleibt hier, der Editor sieht
  // nur Auswahl lesen und Text einfügen.
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
    // Zeilenumbrüche haben in einer einzeiligen Beschriftung nichts verloren.
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
    // Den Zeiger im Seitenraum weitermelden, nach Apply, damit die eben
    // verschobene oder gezoomte Ansicht zählt.
    if (on_pointer_move_) {
      on_pointer_move_(MouseInteraction::ScreenToPage(position_physical, mvp_));
    }
    // Nur Ziehen und Mausrad ändern die Ansicht; blosse Zeigerbewegung löst
    // keinen Repaint aus — ein Hover-Wechsel stösst seinen eigenen über
    // CalendarPage::ReceiveHovered an. Projektion und Zoom-Grenzen bleiben
    // unberührt, RefreshView ist hier nicht nötig.
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
