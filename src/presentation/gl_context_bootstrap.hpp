#ifndef GL_CONTEXT_BOOTSTRAP_HPP
#define GL_CONTEXT_BOOTSTRAP_HPP

#include <epoxy/gl.h>
#include <wx/dcclient.h>
#include <wx/event.h>
#include <wx/glcanvas.h>
#include <wx/string.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

// Procures the canvas its OpenGL context — the only place that does so.
//
// The setup is deferred, because a wxGLCanvas lies on the screen at the first
// paint alone; before that no context can be created. For that the bootstrap
// attaches itself once to wxEVT_PAINT, tries again on every paint and reports
// success or failure exactly once. Afterwards it detaches again and the canvas
// draws.
class GlContextBootstrap {
 public:
  struct Version {
    int major{};
    int minor{};
  };

  GlContextBootstrap(wxGLCanvas& canvas, Version version)
      : canvas_(canvas), version_(version) {}
  ~GlContextBootstrap() = default;
  GlContextBootstrap(const GlContextBootstrap&) = delete;
  GlContextBootstrap& operator=(const GlContextBootstrap&) = delete;
  GlContextBootstrap(GlContextBootstrap&&) = delete;
  GlContextBootstrap& operator=(GlContextBootstrap&&) = delete;

  // `on_failed` is mandatory: without a context the wiring stays out, and an
  // operable but unwired window crashes on the first click.
  void Start(std::function<void()> on_ready,
             std::function<void(const std::string&)> on_failed) {
    on_ready_ = std::move(on_ready);
    on_failed_ = std::move(on_failed);
    canvas_.Bind(wxEVT_PAINT, &GlContextBootstrap::OnInitialPaint, this);
    if (canvas_.IsShownOnScreen()) {
      canvas_.Refresh(false);
    }
  }

  [[nodiscard]] bool IsReady() const { return status_ == Status::kReady; }

  void MakeCurrent() { canvas_.SetCurrent(*context_); }

 private:
  enum class Status : std::uint8_t { kPending, kReady, kFailed };

  void OnInitialPaint(wxPaintEvent& /*event*/) {
    const wxPaintDC paint_dc(&canvas_);
    if (TryCreateContext() == Status::kPending) {
      return;
    }

    canvas_.Unbind(wxEVT_PAINT, &GlContextBootstrap::OnInitialPaint, this);

    if (status_ == Status::kFailed) {
      on_failed_(FailureMessage());
      return;
    }

    on_ready_();
    canvas_.Refresh(false);
  }

  Status TryCreateContext() {
    if (status_ != Status::kPending) {
      return status_;
    }
    if (!canvas_.IsShownOnScreen()) {
      std::cout << "!canvas_shown_on_screen\n";
      return status_;
    }

    wxGLContextAttrs context_attributes;
    context_attributes.PlatformDefaults()
        .CoreProfile()
        .OGLVersion(version_.major, version_.minor)
        .EndList();
    context_ =
        std::make_unique<wxGLContext>(&canvas_, nullptr, &context_attributes);
    std::cout << "context IsOK " << context_->IsOK() << '\n';

    // Without a valid context no GL function may run any more: glGetString
    // would return nullptr, and one more attempt per paint would be an endless
    // loop without a single message.
    if (!context_->IsOK()) {
      context_.reset();
      status_ = Status::kFailed;
      return status_;
    }

    MakeCurrent();
    status_ = Status::kReady;
    std::cout << "OpenGL ready, version: " << DescribeDriver() << '\n';
    return status_;
  }

  [[nodiscard]] std::string FailureMessage() const {
    return "Creating an OpenGL " + std::to_string(version_.major) + "." +
           std::to_string(version_.minor) +
           " core context failed. decade needs OpenGL to draw the calendar and "
           "cannot continue.";
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
    return wxString::FromUTF8(reinterpret_cast<const char*>(value))
        .ToStdString();
  }

  wxGLCanvas& canvas_;
  Version version_;
  Status status_{Status::kPending};
  std::unique_ptr<wxGLContext> context_;
  std::function<void()> on_ready_;
  std::function<void(const std::string&)> on_failed_;
};

#endif  // GL_CONTEXT_BOOTSTRAP_HPP
