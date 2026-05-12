#ifndef HOME_TITAN99_CODE_DECADE_SRC_GUI_OPENGL_PANEL_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GUI_OPENGL_PANEL_HPP

#include <epoxy/gl.h>
#include <wx/glcanvas.h>
#include <wx/weakref.h>
#include <wx/wx.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/io.hpp>
#include <iostream>
#include <memory>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>

namespace decade_debug {
inline bool LogEnabled() {
  static const bool enabled = std::getenv("DECADE_DEBUG_LOG") != nullptr;
  return enabled;
}
inline void LogMat4(const char* tag, const glm::mat4& m) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": diag(" << m[0][0] << "," << m[1][1] << "," << m[2][2]
            << ") trans(" << m[3][0] << "," << m[3][1] << "," << m[3][2]
            << ")\n";
}
inline void LogVec3(const char* tag, const glm::vec3& v) {
  if (!LogEnabled()) {
    return;
  }
  std::cout << tag << ": (" << v.x << "," << v.y << "," << v.z << ")\n";
}
}  // namespace decade_debug

#include "../graphics/graphics_engine.hpp"
#include "../graphics/mvp_matrices.hpp"
#include "../graphics/projection.hpp"
#include "../graphics/render_to_png.hpp"
#include "../packages/page_config.hpp"

// Funktion zur Umwandlung der OpenGL-Enums in menschenlesbare Strings
const char* GetSourceString(GLenum source) {
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

const char* GetTypeString(GLenum type) {
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

const char* GetSeverityString(GLenum severity) {
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
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei /*length*/,
                            const GLchar* message, const void* /*userParam*/) {
  std::cout << "OpenGL Debug Message:";
  std::cout << "  Source: " << GetSourceString(source);
  std::cout << ", Type: " << GetTypeString(type);
  std::cout << ", ID: " << std::hex << id << std::dec;
  std::cout << ", Severity: " << GetSeverityString(severity);
  std::cout << ", Message: " << message;
  std::cout << '\n';
}

class MouseInteraction {
 public:
  MouseInteraction()
      : persistent_mouse_pos(0.F),
        translate_pre_scaled(0.F),
        translate_post_scaled(0.F) {}

  void Interaction(MVP& mvp, wxPoint mouse_position, bool dragging,
                   int wheel_rotation) {
    glm::vec3 const current_mouse_pos = MouseWorldSpacePos(mouse_position, mvp);

    if (decade_debug::LogEnabled()) {
      std::cout << "Mouse: px=(" << mouse_position.x << "," << mouse_position.y
                << ") drag=" << dragging << " wheel=" << wheel_rotation << '\n';
      decade_debug::LogVec3("Mouse current (view-space)", current_mouse_pos);
      decade_debug::LogVec3("Mouse persistent (prev)", persistent_mouse_pos);
    }

    if (dragging) {
      translate_pre_scaled += current_mouse_pos - persistent_mouse_pos;
    }

    if (wheel_rotation != 0) {
      const float mouse_wheel_step = 1200.F;
      const auto scale = static_cast<float>(wheel_rotation) / mouse_wheel_step;

      const auto pre_scale_view_matrix =
          CalculateViewMatrix(persistent_scale_factor);
      const auto pre_scale_mouse_pos =
          MouseViewSpacePos(current_mouse_pos, pre_scale_view_matrix);

      persistent_scale_factor *= std::exp(scale);

      const auto post_scale_view_matrix =
          CalculateViewMatrix(persistent_scale_factor);
      const auto post_scale_mouse_pos =
          MouseViewSpacePos(current_mouse_pos, post_scale_view_matrix);

      const glm::vec3 view_space_correction =
          post_scale_mouse_pos - pre_scale_mouse_pos;
      translate_post_scaled += view_space_correction;
    }

    auto view_matrix = CalculateViewMatrix(persistent_scale_factor);
    mvp.SetView(view_matrix);

    if (decade_debug::LogEnabled()) {
      decade_debug::LogVec3("translate_pre_scaled", translate_pre_scaled);
      decade_debug::LogVec3("translate_post_scaled", translate_post_scaled);
      std::cout << "scale=" << persistent_scale_factor << '\n';
      decade_debug::LogMat4("view_matrix", view_matrix);
    }

    persistent_mouse_pos = current_mouse_pos;
  }

 private:
  glm::mat4 CalculateViewMatrix(float scale_factor) {
    auto pre_scaled = glm::translate(glm::mat4(1.F), translate_pre_scaled);
    auto post_scaled =
        glm::scale(pre_scaled, glm::vec3(scale_factor, scale_factor, 1.F));
    auto view_matrix = glm::translate(post_scaled, translate_post_scaled);
    return view_matrix;
  }

  static glm::vec3 MouseClipSpace(const wxPoint& mouse_pos_px) {
    const glm::vec2 window_mouse_pos(static_cast<float>(mouse_pos_px.x),
                                     static_cast<float>(mouse_pos_px.y));

    std::array<GLint, 4> viewport_px;
    glGetIntegerv(GL_VIEWPORT, viewport_px.data());
    glm::vec4 const viewport(0.F, 0.F, static_cast<float>(viewport_px[2]),
                             static_cast<float>(viewport_px[3]));

    auto viewport_ortho =
        glm::ortho(viewport.x, viewport.z, viewport.w, viewport.y);
    auto mouse_pos_clip_space =
        viewport_ortho * glm::vec4(window_mouse_pos, 0.F, 1.F);
    return mouse_pos_clip_space;
  }

  glm::vec3 MouseWorldSpacePos(const wxPoint& mouse_pos_px, const MVP& mvp) {
    const glm::mat4 projection_matrix = mvp.GetProjection();
    const auto inverse_projection_matrix = glm::inverse(projection_matrix);

    const auto mouse_pos = inverse_projection_matrix *
                           glm::vec4(MouseClipSpace(mouse_pos_px), 1.F);
    return {mouse_pos.x, mouse_pos.y, 0.F};
  }

  static glm::vec3 MouseViewSpacePos(const glm::vec3& mouse_world_space_pos,
                                     const glm::mat4& view_matrix) {
    const auto inverse_view_matrix = glm::inverse(view_matrix);

    const auto mouse_pos =
        inverse_view_matrix * glm::vec4(mouse_world_space_pos, 1.F);
    return {mouse_pos.x, mouse_pos.y, 0.F};
  }

  float persistent_scale_factor{1.F};
  glm::vec3 persistent_mouse_pos;
  glm::vec3 translate_pre_scaled;
  glm::vec3 translate_post_scaled;
};

class GLCanvas {
 public:
  explicit GLCanvas(wxWindow* parent) {
    wxGLAttributes attributes;
    attributes.PlatformDefaults().Defaults().EndList();
    bool const display_supported = wxGLCanvas::IsDisplaySupported(attributes);
    std::cout << "wxGLCanvas IsDisplaySupported " << std::boolalpha
              << display_supported << '\n';
    wx_gl_canvas = std::make_unique<wxGLCanvas>(parent, attributes).release();
  }

  wxGLCanvas* GLCanvasPtr() { return wx_gl_canvas.get(); }

  GraphicsEngine* GraphicsEnginePtr() { return graphics_engine.get(); }

  // Verzögerter GL-Init: bindet wxEVT_PAINT auf einen einmaligen
  // Init-Handler. Beim ersten Paint ist das Canvas garantiert gemappt,
  // dann lädt LoadOpenGL und on_ready wird aufgerufen.
  void InitOpenGL(const std::array<int, 2>& version,
                  std::function<void()> on_ready) {
    gl_version_ = version;
    on_gl_ready_ = std::move(on_ready);
    wx_gl_canvas->Bind(wxEVT_PAINT, &GLCanvas::InitPaintCallback, this);
    if (wx_gl_canvas->IsShownOnScreen()) {
      wx_gl_canvas->Refresh(false);
    }
  }

  int LoadOpenGL(const std::array<int, 2>& version) {
    bool const canvas_shown_on_screen = wx_gl_canvas->IsShownOnScreen();

    if (!canvas_shown_on_screen) {
      std::cout << "!canvas_shown_on_screen\n";
    }

    if (gl_loaded == 0 && canvas_shown_on_screen) {
      wxGLContextAttrs context_attributes;
      context_attributes.PlatformDefaults()
          .CoreProfile()
          .OGLVersion(version[0], version[1])
          .EndList();
      context = std::make_unique<wxGLContext>(wx_gl_canvas, nullptr,
                                              &context_attributes);
      std::cout << "context IsOK " << context->IsOK() << '\n';

      wx_gl_canvas->SetCurrent(*context);
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

      wx_gl_canvas->Bind(wxEVT_SIZE, &GLCanvas::SizeCallback, this);
      wx_gl_canvas->Bind(wxEVT_PAINT, &GLCanvas::PaintCallback, this);

      mouse_interaction = std::make_unique<MouseInteraction>();

      wx_gl_canvas->Bind(wxEVT_MOTION, &GLCanvas::MouseCallback, this);
      wx_gl_canvas->Bind(wxEVT_LEFT_DOWN, &GLCanvas::MouseCallback, this);
      wx_gl_canvas->Bind(wxEVT_LEFT_UP, &GLCanvas::MouseCallback, this);
      wx_gl_canvas->Bind(wxEVT_MOUSEWHEEL, &GLCanvas::MouseCallback, this);
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

    const wxSize logical_size = wx_gl_canvas->GetClientSize();
    const double scale = wx_gl_canvas->GetContentScaleFactor();
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

    wx_gl_canvas->Refresh(false);
  }

  void SavePNG(std::string file_path) {
    const int dpi = 600;
    const int msaa_samples = 16;
    RenderToPNG const render_to_png(std::move(file_path), page_size, dpi,
                                    graphics_engine, msaa_samples);
  }

  // Dumps the current window framebuffer (what is actually on screen) to PNG.
  // Uses glReadPixels on the back buffer; flips y because OpenGL origin is
  // bottom-left while PNG is top-left.
  void SaveWindowPNG(const std::string& file_path) {
    wx_gl_canvas->SetCurrent(*context);
    graphics_engine->SetMVP(mvp);
    graphics_engine->Render();
    glFinish();

    const wxSize logical_size = wx_gl_canvas->GetClientSize();
    const double scale = wx_gl_canvas->GetContentScaleFactor();
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
  void InitPaintCallback(wxPaintEvent& /*event*/) {
    wxPaintDC const dc(wx_gl_canvas);
    if (LoadOpenGL(gl_version_) != 0) {
      wx_gl_canvas->Unbind(wxEVT_PAINT, &GLCanvas::InitPaintCallback, this);
      if (on_gl_ready_) {
        auto cb = std::move(on_gl_ready_);
        cb();
      }
      wx_gl_canvas->Refresh(false);
    }
  }

  void PaintCallback(wxPaintEvent& /*event*/) {
    wxPaintDC const dc(wx_gl_canvas);
    graphics_engine->SetMVP(mvp);
    graphics_engine->Render();
    wx_gl_canvas->SwapBuffers();
  }

  void SizeCallback(wxSizeEvent& /*event*/) {
    RefreshMVP();
    wx_gl_canvas->Refresh(false);
  }

  void MouseCallback(wxMouseEvent& event) {
    const double scale = wx_gl_canvas->GetContentScaleFactor();
    const wxPoint pos_physical(
        static_cast<int>(std::lround(event.GetPosition().x * scale)),
        static_cast<int>(std::lround(event.GetPosition().y * scale)));
    mouse_interaction->Interaction(mvp, pos_physical, event.Dragging(),
                                   event.GetWheelRotation());
    RefreshMVP();
  }

  int gl_loaded{0};

  wxWeakRef<wxGLCanvas> wx_gl_canvas;

  std::unique_ptr<wxGLContext> context;

  std::shared_ptr<GraphicsEngine> graphics_engine;
  std::unique_ptr<MouseInteraction> mouse_interaction;

  rectf page_size;
  MVP mvp;

  std::array<int, 2> gl_version_{};
  std::function<void()> on_gl_ready_;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GUI_OPENGL_PANEL_HPP
