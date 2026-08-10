#ifndef RENDER_TO_TEXTURE_HPP
#define RENDER_TO_TEXTURE_HPP

#include <epoxy/gl.h>

#include <memory>
#include <vector>

#include "texture_object.hpp"

// Restores the framebuffer that was bound on entry. Unbinding to 0 instead
// would be a guess about who owns the screen: 0 is the window's back buffer
// only under a context that draws into one. A QOpenGLWidget renders into a
// framebuffer object of its own, so 0 there is not the target the caller came
// from, and everything drawn afterwards would land nowhere — silently.
class ScopedFramebufferBinding {
 public:
  ScopedFramebufferBinding();

  ~ScopedFramebufferBinding();

  ScopedFramebufferBinding(const ScopedFramebufferBinding&) = delete;
  ScopedFramebufferBinding& operator=(const ScopedFramebufferBinding&) = delete;
  ScopedFramebufferBinding(ScopedFramebufferBinding&&) = delete;
  ScopedFramebufferBinding& operator=(ScopedFramebufferBinding&&) = delete;

 private:
  GLuint previous_{0};
};

// Owns a single OpenGL renderbuffer handle. Non-copyable/non-movable because
// the destructor deletes the handle — a copy would delete it twice.
class RenderBuffer {
 public:
  RenderBuffer();

  ~RenderBuffer();

  RenderBuffer(const RenderBuffer&) = delete;
  RenderBuffer& operator=(const RenderBuffer&) = delete;
  RenderBuffer(RenderBuffer&&) = delete;
  RenderBuffer& operator=(RenderBuffer&&) = delete;

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};

// Owns a single OpenGL framebuffer plus its colour texture and depth
// renderbuffer. Non-copyable/non-movable for the same reason as RenderBuffer.
class FrameBuffer {
 public:
  FrameBuffer(GLsizei width, GLsizei height, GLsizei samples, bool msaa);

  ~FrameBuffer();

  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer& operator=(const FrameBuffer&) = delete;
  FrameBuffer(FrameBuffer&&) = delete;
  FrameBuffer& operator=(FrameBuffer&&) = delete;

  [[nodiscard]] GLenum CheckStatus() const;

  [[nodiscard]] GLuint Name() const;

  [[nodiscard]] GLuint TextureName() const;

 private:
  GLuint name_{0};
  Texture texture_;
  RenderBuffer render_buffer_;
};

// What this implementation can actually multisample, for an RGBA colour texture
// with a depth renderbuffer beside it. Asking for more is not a warning but a
// failure: glTexImage2DMultisample raises GL_INVALID_VALUE past GL_MAX_SAMPLES
// and GL_INVALID_OPERATION past GL_MAX_COLOR_TEXTURE_SAMPLES, the texture never
// comes into being, the framebuffer stays incomplete — and everything drawn
// into it is lost without a word. That is how the 16 samples of the PNG export
// produced a black page under llvmpipe, whose ceiling is 8 (#49).
[[nodiscard]] GLsizei MaxUsableSamples();

// Renders into an off-screen framebuffer and reads the result back as RGBA
// bytes. With multisampling (samples > 1) it renders into a dedicated MSAA
// framebuffer and resolves it into the readable output buffer; without it,
// rendering goes straight into the output buffer and the extra MSAA buffer is
// never allocated.
//
// The wanted sample count is a wish, not a demand: it gets capped at what the
// driver offers, and should the MSAA framebuffer still not come up, the render
// falls back to a single sample. A page without antialiasing beats a black one.
class RenderToTexture {
 public:
  RenderToTexture(GLsizei width_in, GLsizei height_in, GLsizei samples_in);

  // False when even the single-sample output framebuffer would not come up.
  // Whatever gets read back then says nothing — the caller has to report it
  // rather than write the bytes out.
  [[nodiscard]] bool Valid() const;

  // The samples the render really runs with, after the cap and a possible
  // fallback — not what the caller asked for.
  [[nodiscard]] GLsizei Samples() const;

  void BeginRender();

  void EndRender();

  std::vector<unsigned char> CopyImage();

 private:
  static constexpr GLsizei kBytesPerPixel = 4;

  GLsizei width_;
  GLsizei height_;
  GLsizei samples_;
  bool multisampled_;

  FrameBuffer output_frame_buffer_;
  std::unique_ptr<FrameBuffer> multisample_frame_buffer_;

  GLint previous_viewport_width_{0};
  GLint previous_viewport_height_{0};
  GLuint previous_framebuffer_{0};

  GLsizei bytes_per_pixel_{kBytesPerPixel};
  std::vector<unsigned char> image_;

  bool valid_{false};
};
#endif  // RENDER_TO_TEXTURE_HPP
