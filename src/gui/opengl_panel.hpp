#ifndef OPENGL_PANEL_HPP
#define OPENGL_PANEL_HPP

#include <epoxy/gl.h>
#include <wx/glcanvas.h>
#include <wx/wx.h>

#include <array>
#include <cmath>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../graphics/debug_log.hpp"
#include "../graphics/graphics_engine.hpp"
#include "../graphics/mvp_matrices.hpp"
#include "../graphics/projection.hpp"
#include "../graphics/render_to_png.hpp"
#include "../packages/page_config.hpp"
#include "mouse_interaction.hpp"

// Funktion zur Umwandlung der OpenGL-Enums in menschenlesbare Strings
inline const char* GetSourceString(GLenum source) {
  switch (source) {
    case GL_DEBUG_SOURCE_API:
      return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      return "Window System";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      return "Shader Compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      return "Third Party";
    case GL_DEBUG_SOURCE_APPLICATION:
      return "Application";
    case GL_DEBUG_SOURCE_OTHER:
      return "Other";
    default:
      return "Unknown";
  }
}

inline const char* GetTypeString(GLenum type) {
  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "Deprecated Behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "Undefined Behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
      return "Portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
      return "Performance";
    case GL_DEBUG_TYPE_OTHER:
      return "Other";
    default:
      return "Unknown";
  }
}

inline const char* GetSeverityString(GLenum severity) {
  switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
      return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:
      return "Medium";
    case GL_DEBUG_SEVERITY_LOW:
      return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      return "Notification";
    default:
      return "Unknown";
  }
}

// Debug Callback-Funktion
inline void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                                   GLenum severity, GLsizei /*length*/,
                                   const GLchar* message,
                                   const void* /*userParam*/) {
  std::cout << "OpenGL Debug Message:";
  std::cout << "  Source: " << GetSourceString(source);
  std::cout << ", Type: " << GetTypeString(type);
  std::cout << ", ID: " << std::hex << id << std::dec;
  std::cout << ", Severity: " << GetSeverityString(severity);
  std::cout << ", Message: " << message;
  std::cout << '\n';
}

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

  GraphicsEngine* GraphicsEnginePtr() { return graphics_engine.get(); }

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

    if (gl_loaded == 0 && canvas_shown_on_screen) {
      wxGLContextAttrs context_attributes;
      context_attributes.PlatformDefaults()
          .CoreProfile()
          .OGLVersion(version[0], version[1])
          .EndList();
      context =
          std::make_unique<wxGLContext>(this, nullptr, &context_attributes);
      std::cout << "context IsOK " << context->IsOK() << '\n';

      SetCurrent(*context);
      gl_loaded = context->IsOK() ? 1 : 0;

      std::cout << "OpenGL ready: " << gl_loaded
                << " version: " << GetGLVersionString() << '\n';

      // glEnable(GL_DEBUG_OUTPUT);
      // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

      // glDisable(GL_DEBUG_OUTPUT);
      // glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

      // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
      // nullptr, GL_TRUE);

      // glDebugMessageCallback(DebugCallback, nullptr);

      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_MULTISAMPLE);
      // glEnable(GL_FRAMEBUFFER_SRGB);

      GLint sample_buffers = 0;
      glGetIntegerv(GL_SAMPLES, &sample_buffers);
      std::cout << "msaa_sample_buffers " << sample_buffers << '\n';

      graphics_engine = std::make_unique<GraphicsEngine>();

      Bind(wxEVT_SIZE, &GLCanvas::SizeCallback, this);
      Bind(wxEVT_PAINT, &GLCanvas::PaintCallback, this);

      mouse_interaction = std::make_unique<MouseInteraction>();

      Bind(wxEVT_MOTION, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_LEFT_DOWN, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_LEFT_UP, &GLCanvas::MouseCallback, this);
      Bind(wxEVT_MOUSEWHEEL, &GLCanvas::MouseCallback, this);
    }

    return gl_loaded;
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
    page_size = rectf::from_dimension(
        rectf::Dimension{.width = page_setup_config.size[0],
                         .height = page_setup_config.size[1]});
    if (decade_debug::LogEnabled()) {
      std::cout << "ReceivePageSetup: page=" << page_size.width() << "x"
                << page_size.height() << " rect=(" << page_size.l() << ","
                << page_size.r() << "," << page_size.b() << "," << page_size.t()
                << ")\n";
    }
    if (graphics_engine) {
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

    if (page_size.width() <= 0.0F || page_size.height() <= 0.0F) {
      if (decade_debug::LogEnabled()) {
        std::cout << "RefreshMVP: skipped, page_size not yet initialised\n";
      }
      return;
    }

    rectf const view_size = page_size.scale(view_size_scale);
    mvp.SetProjection(Projection::OrthoMatrix(view_size));

    graphics_engine->SetMVP(mvp);

    if (decade_debug::LogEnabled()) {
      std::cout << "RefreshMVP: viewport=" << size.GetWidth() << "x"
                << size.GetHeight() << " page=" << page_size.width() << "x"
                << page_size.height() << " view=" << view_size.width() << "x"
                << view_size.height() << '\n';
      decade_debug::LogMat4("RefreshMVP proj", mvp.GetProjection());
      decade_debug::LogMat4("RefreshMVP view", mvp.GetView());
    }

    Refresh(false);
  }

  void SavePNG(std::string file_path) {
    RenderToPNG const render_to_png(std::move(file_path), page_size,
                                    kExportPngDpi, graphics_engine,
                                    kExportMsaaSamples);
  }

  // Dumps the current window framebuffer (what is actually on screen) to PNG.
  // Uses glReadPixels on the back buffer; flips y because OpenGL origin is
  // bottom-left while PNG is top-left.
  void SaveWindowPNG(const std::string& file_path) {
    SetCurrent(*context);
    graphics_engine->SetMVP(mvp);
    graphics_engine->Render();
    glFinish();

    const wxSize logical_size = GetClientSize();
    const double scale = GetContentScaleFactor();
    const auto w =
        static_cast<size_t>(std::lround(logical_size.GetWidth() * scale));
    const auto h =
        static_cast<size_t>(std::lround(logical_size.GetHeight() * scale));
    if (w == 0 || h == 0) {
      return;
    }
    constexpr size_t kBytesPerPixel = 4;
    std::vector<unsigned char> buffer(w * h * kBytesPerPixel);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                 GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    // flip vertically
    std::vector<unsigned char> flipped(buffer.size());
    const size_t row_bytes = w * kBytesPerPixel;
    for (size_t y = 0; y < h; ++y) {
      std::copy_n(buffer.data() + ((h - 1 - y) * row_bytes), row_bytes,
                  flipped.data() + (y * row_bytes));
    }

    RenderToPNG::SaveRgbaPng(file_path.c_str(), flipped, w, h);
  }

 private:
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
    graphics_engine->SetMVP(mvp);
    graphics_engine->Render();
    SwapBuffers();
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
    mouse_interaction->Apply(mvp, pos_physical, event.Dragging(),
                             event.GetWheelRotation());
    RefreshMVP();
  }

  int gl_loaded{0};

  std::unique_ptr<wxGLContext> context;

  std::shared_ptr<GraphicsEngine> graphics_engine;
  std::unique_ptr<MouseInteraction> mouse_interaction;

  rectf page_size;
  MVP mvp;

  std::array<int, 2> gl_version_{};
  std::function<void()> on_gl_ready_;
};
#endif  // OPENGL_PANEL_HPP
