#ifndef GL_CANVAS_HPP
#define GL_CANVAS_HPP

#include <epoxy/gl.h>
#include <wx/glcanvas.h>
#include <wx/image.h>
#include <wx/wx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../common/debug_log.hpp"
#include "../domain/page_setup_config.hpp"
#include "../infrastructure/graphics/frame_stats.hpp"
#include "../infrastructure/graphics/graphics_engine.hpp"
#include "../infrastructure/graphics/mvp_matrices.hpp"
#include "../infrastructure/graphics/pan_zoom_camera.hpp"
#include "../infrastructure/graphics/png_writer.hpp"
#include "../infrastructure/graphics/projection.hpp"
#include "../infrastructure/graphics/render_to_png.hpp"
#include "mouse_interaction.hpp"

class GLCanvas : public wxGLCanvas {
 public:
  // Resolution and multisampling used by the PNG export (SavePNG). Kept public
  // so the menu label can be derived from the same value — single source of
  // truth instead of repeating the number in a hard-coded string.
  static constexpr int kExportPngDpi = 200;
  static constexpr int kExportMsaaSamples = 16;

  explicit GLCanvas(wxWindow* parent)
      : wxGLCanvas(parent, DisplayAttributes()) {
    std::cout << "wxGLCanvas IsDisplaySupported " << std::boolalpha
              << wxGLCanvas::IsDisplaySupported(DisplayAttributes()) << '\n';
  }

  GraphicsEngine* GraphicsEnginePtr() { return graphics_engine_.get(); }

  // Called on every mouse move with the cursor in page/world space, so an
  // interaction controller can hit-test it. Set by the binder.
  void SetPointerMoveCallback(std::function<void(glm::vec2)> callback) {
    on_pointer_move_ = std::move(callback);
  }

  // Verzögerter GL-Init: bindet wxEVT_PAINT auf einen einmaligen
  // Init-Handler. Beim ersten Paint ist das Canvas garantiert gemappt,
  // dann lädt LoadOpenGL und on_ready wird aufgerufen.
  void InitOpenGL(const std::array<int, 2>& version,
                  std::function<void()> on_ready) {
    gl_version_ = version;
    on_gl_ready_ = std::move(on_ready);
    Bind(wxEVT_PAINT, &GLCanvas::InitPaintCallback, this);
    if (IsShownOnScreen()) {
      Refresh(false);
    }
  }

  int LoadOpenGL(const std::array<int, 2>& version) {
    bool const canvas_shown_on_screen = IsShownOnScreen();

    if (!canvas_shown_on_screen) {
      std::cout << "!canvas_shown_on_screen\n";
    }

    if (gl_loaded_ == 0 && canvas_shown_on_screen) {
      wxGLContextAttrs context_attributes;
      context_attributes.PlatformDefaults()
          .CoreProfile()
          .OGLVersion(version[0], version[1])
          .EndList();
      context_ =
          std::make_unique<wxGLContext>(this, nullptr, &context_attributes);
      std::cout << "context IsOK " << context_->IsOK() << '\n';

      SetCurrent(*context_);
      gl_loaded_ = context_->IsOK() ? 1 : 0;

      std::cout << "OpenGL ready: " << gl_loaded_
                << " version: " << GetGLVersionString() << '\n';

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_MULTISAMPLE);

      GLint sample_buffers = 0;
      glGetIntegerv(GL_SAMPLES, &sample_buffers);
      std::cout << "msaa_sample_buffers " << sample_buffers << '\n';

      graphics_engine_ = std::make_unique<GraphicsEngine>();

      Bind(wxEVT_SIZE, &GLCanvas::SizeCallback, this);
      Bind(wxEVT_PAINT, &GLCanvas::PaintCallback, this);

      mouse_interaction_ = std::make_unique<MouseInteraction>();

      Bind(wxEVT_MOTION, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_LEFT_DOWN, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_LEFT_UP, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_MOUSEWHEEL, &GLCanvas::MouseCallback, this);
    }

    return gl_loaded_;
  }

  static std::string GetGLVersionString() {
    std::string version_string;
    version_string += "GL_VERSION " +
                      wxString::FromUTF8(reinterpret_cast<const char*>(
                          glGetString(GL_VERSION))) +
                      '\n';
    version_string += "GL_VENDOR " +
                      wxString::FromUTF8(reinterpret_cast<const char*>(
                          glGetString(GL_VENDOR))) +
                      '\n';
    version_string +=
        "GL_RENDERER " + wxString::FromUTF8(reinterpret_cast<const char*>(
                             glGetString(GL_RENDERER)));
    return version_string;
  }

  void ReceivePageSetup(const PageSetupConfig& page_setup_config) {
    page_size_ = rectf::from_dimension(
        rectf::Dimension{.width = page_setup_config.Size()[0],
                         .height = page_setup_config.Size()[1]});
    if (decade_debug::LogEnabled()) {
      std::cout << "ReceivePageSetup: page=" << page_size_.width() << "x"
                << page_size_.height() << " rect=(" << page_size_.l() << ","
                << page_size_.r() << "," << page_size_.b() << ","
                << page_size_.t() << ")\n";
    }
    if (graphics_engine_) {
      RefreshMVP();
    }
  }

  void RefreshMVP() {
    const float view_size_scale = 1.1F;

    const wxSize logical_size = GetClientSize();
    const double scale = GetContentScaleFactor();
    const auto fb_width =
        static_cast<GLsizei>(std::lround(logical_size.GetWidth() * scale));
    const auto fb_height =
        static_cast<GLsizei>(std::lround(logical_size.GetHeight() * scale));
    glViewport(0, 0, fb_width, fb_height);

    if (decade_debug::LogEnabled()) {
      std::cout << "RefreshMVP scale=" << scale
                << " logical=" << logical_size.GetWidth() << "x"
                << logical_size.GetHeight() << " fb=" << fb_width << "x"
                << fb_height << '\n';
    }

    const wxSize size(static_cast<int>(fb_width), static_cast<int>(fb_height));

    if (page_size_.width() <= 0.0F || page_size_.height() <= 0.0F) {
      if (decade_debug::LogEnabled()) {
        std::cout << "RefreshMVP: skipped, page_size not yet initialised\n";
      }
      return;
    }

    rectf const view_size = page_size_.scale(view_size_scale);
    mvp_.SetProjection(Projection::OrthoMatrix(view_size));
    camera_.SetScaleLimits(ComputeZoomLimits(
        mvp_.GetProjection(), {page_size_.width(), page_size_.height()},
        static_cast<float>(kExportPngDpi)));

    graphics_engine_->SetMVP(mvp_);

    if (decade_debug::LogEnabled()) {
      std::cout << "RefreshMVP: viewport=" << size.GetWidth() << "x"
                << size.GetHeight() << " page=" << page_size_.width() << "x"
                << page_size_.height() << " view=" << view_size.width() << "x"
                << view_size.height() << '\n';
      decade_debug::LogMat4("RefreshMVP proj", mvp_.GetProjection());
      decade_debug::LogMat4("RefreshMVP view", mvp_.GetView());
    }

    Refresh(false);
  }

  // Stösst nur einen Repaint an — für Änderungen, die weder Projektion noch
  // Zoom-Grenzen berühren (Hover-/Selektionsfarben). Deutlich billiger als
  // RefreshMVP.
  void Repaint() { Refresh(false); }

  // Bildrate im Sekundenfenster des jüngsten Frames; da nur ereignisgesteuert
  // gezeichnet wird, ist der Wert während einer Interaktion aussagekräftig.
  [[nodiscard]] double CurrentFps() const { return frame_stats_.Fps(); }

  void SavePNG(std::string file_path, int dpi = kExportPngDpi) {
    RenderToPNG const render_to_png(std::move(file_path), page_size_,
                                    static_cast<float>(dpi), graphics_engine_,
                                    kExportMsaaSamples);
  }

  // Dumps the current window framebuffer (what is actually on screen) to PNG.
  void SaveWindowPNG(const std::string& file_path) {
    BackBuffer back = ReadBackBuffer(GL_RGBA, 4);
    if (back.pixels.empty()) {
      return;
    }
    png_io::WriteRgbaPng(
        file_path.c_str(), back.pixels,
        png_io::PngImageSize{.width = back.width, .height = back.height});
  }

  // Returns the current GL back buffer as a top-left-origin RGB wxImage so it
  // can be composited into a full-window screenshot. A wxDC cannot read the GL
  // surface directly, so callers that screenshot the whole frame paste this on
  // top of the GL canvas region. Returns an invalid image when the canvas has
  // no area yet.
  wxImage CaptureBackBufferImage() {
    const BackBuffer back = ReadBackBuffer(GL_RGB, 3);
    if (back.pixels.empty()) {
      return {};
    }
    wxImage image(static_cast<int>(back.width), static_cast<int>(back.height));
    std::copy_n(back.pixels.data(), back.pixels.size(), image.GetData());
    return image;
  }

 private:
  struct BackBuffer {
    std::vector<unsigned char> pixels;  // top-left-origin, already row-flipped
    size_t width{0};
    size_t height{0};
  };

  // Renders the scene to the back buffer and reads it back as a top-left-origin
  // pixel buffer in the given GL format (GL_RGB/GL_RGBA, `bytes_per_pixel`
  // matching). OpenGL's origin is bottom-left, so the rows are flipped. Returns
  // an empty buffer when the canvas has no area yet.
  BackBuffer ReadBackBuffer(GLenum format, size_t bytes_per_pixel) {
    SetCurrent(*context_);
    graphics_engine_->SetMVP(mvp_);
    graphics_engine_->Render();
    glFinish();

    const wxSize logical_size = GetClientSize();
    const double scale = GetContentScaleFactor();
    const auto width =
        static_cast<size_t>(std::lround(logical_size.GetWidth() * scale));
    const auto height =
        static_cast<size_t>(std::lround(logical_size.GetHeight() * scale));
    if (width == 0 || height == 0) {
      return {};
    }
    std::vector<unsigned char> buffer(width * height * bytes_per_pixel);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, static_cast<GLsizei>(width),
                 static_cast<GLsizei>(height), format, GL_UNSIGNED_BYTE,
                 buffer.data());

    std::vector<unsigned char> flipped(buffer.size());
    const size_t row_bytes = width * bytes_per_pixel;
    for (size_t y = 0; y < height; ++y) {
      std::copy_n(buffer.data() + ((height - 1 - y) * row_bytes), row_bytes,
                  flipped.data() + (y * row_bytes));
    }
    return {.pixels = std::move(flipped), .width = width, .height = height};
  }

  static wxGLAttributes DisplayAttributes() {
    wxGLAttributes attributes;
    attributes.PlatformDefaults().Defaults().EndList();
    return attributes;
  }

  void InitPaintCallback(wxPaintEvent& /*event*/) {
    wxPaintDC const dc(this);
    if (LoadOpenGL(gl_version_) != 0) {
      Unbind(wxEVT_PAINT, &GLCanvas::InitPaintCallback, this);
      if (on_gl_ready_) {
        auto cb = std::move(on_gl_ready_);
        cb();
      }
      Refresh(false);
    }
  }

  void PaintCallback(wxPaintEvent& /*event*/) {
    wxPaintDC const dc(this);
    const auto render_start = FrameStats::Clock::now();
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

  void SizeCallback(wxSizeEvent& /*event*/) {
    RefreshMVP();
    Refresh(false);
  }

  void MouseCallback(wxMouseEvent& event) {
    const double scale = GetContentScaleFactor();
    const wxPoint pos_physical(
        static_cast<int>(std::lround(event.GetPosition().x * scale)),
        static_cast<int>(std::lround(event.GetPosition().y * scale)));
    mouse_interaction_->Apply(mvp_, camera_, pos_physical, event.Dragging(),
                              event.GetWheelRotation());
    // Forward the cursor in page/world space for hit-testing. Computed after
    // Apply so it uses the current (possibly just panned/zoomed) view.
    if (on_pointer_move_) {
      on_pointer_move_(MouseInteraction::ScreenToPage(pos_physical, mvp_));
    }
    // Nur Ziehen und Mausrad ändern die Ansicht; blosse Zeigerbewegung löst
    // keinen Repaint aus — ein Hover-Wechsel stösst seinen eigenen über
    // CalendarPage::ReceiveHovered an. Projektion und Zoom-Grenzen bleiben
    // unberührt, RefreshMVP ist hier nicht nötig.
    if (event.Dragging() || event.GetWheelRotation() != 0) {
      Repaint();
    }
  }

  int gl_loaded_{0};

  std::unique_ptr<wxGLContext> context_;

  std::shared_ptr<GraphicsEngine> graphics_engine_;
  std::unique_ptr<MouseInteraction> mouse_interaction_;
  PanZoomCamera camera_;

  rectf page_size_;
  MVP mvp_;

  FrameStats frame_stats_;
  FrameStats::Clock::time_point last_fps_log_;

  std::array<int, 2> gl_version_{};
  std::function<void()> on_gl_ready_;
  std::function<void(glm::vec2)> on_pointer_move_;
};
#endif  // GL_CANVAS_HPP
