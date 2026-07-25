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

// Beschafft dem Canvas seinen OpenGL-Kontext — der einzige Ort, der das tut.
//
// Der Aufbau ist verzögert, weil wxGLCanvas erst beim ersten Paint auf dem
// Bildschirm liegt; vorher lässt sich kein Kontext erzeugen. Der Bootstrap
// hängt sich dafür einmalig an wxEVT_PAINT, versucht es bei jedem Paint erneut
// und meldet genau einmal Erfolg oder Scheitern. Danach hängt er sich wieder
// ab und das Canvas zeichnet.
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

  // `on_failed` ist Pflicht: ohne Kontext bleibt die Verdrahtung aus, und ein
  // bedienbares, aber unverdrahtetes Fenster stürzt beim ersten Klick ab.
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

    // Ohne gültigen Kontext darf keine GL-Funktion mehr laufen: glGetString
    // gäbe nullptr zurück, und ein neuer Versuch pro Paint wäre eine
    // Endlosschleife ohne jede Meldung.
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

  // Nur mit aktuellem Kontext gültig; ohne einen gibt glGetString nullptr
  // zurück, deshalb der Fallback statt eines Casts ins Leere.
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
