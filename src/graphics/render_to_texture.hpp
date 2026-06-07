#ifndef RENDER_TO_TEXTURE_HPP
#define RENDER_TO_TEXTURE_HPP

#include <epoxy/gl.h>

#include <array>
#include <memory>
#include <vector>

#include "texture_object.hpp"

// Owns a single OpenGL renderbuffer handle. Non-copyable/non-movable because
// the destructor deletes the handle — a copy would delete it twice.
class RenderBuffer {
 public:
  RenderBuffer() { glGenRenderbuffers(1, &name); }

  ~RenderBuffer() { glDeleteRenderbuffers(1, &name); }

  RenderBuffer(const RenderBuffer&) = delete;
  RenderBuffer& operator=(const RenderBuffer&) = delete;
  RenderBuffer(RenderBuffer&&) = delete;
  RenderBuffer& operator=(RenderBuffer&&) = delete;

  [[nodiscard]] GLuint Name() const { return name; }

 private:
  GLuint name{0};
};

// Owns a single OpenGL framebuffer plus its colour texture and depth
// renderbuffer. Non-copyable/non-movable for the same reason as RenderBuffer.
class FrameBuffer {
 public:
  FrameBuffer(GLsizei width, GLsizei height, GLsizei samples, bool msaa) {
    glGenFramebuffers(1, &name);

    if (!msaa) {
      glBindTexture(GL_TEXTURE_2D, texture.Name());
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, nullptr);
      glBindTexture(GL_TEXTURE_2D, 0);

      glBindRenderbuffer(GL_RENDERBUFFER, render_buffer.Name());
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width,
                            height);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);

      glBindFramebuffer(GL_FRAMEBUFFER, name);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER, render_buffer.Name());
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, texture.Name(), 0);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (msaa) {
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture.Name());
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8,
                              width, height, GL_TRUE);
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

      glBindRenderbuffer(GL_RENDERBUFFER, render_buffer.Name());
      glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                       GL_DEPTH_COMPONENT24, width, height);
      glBindRenderbuffer(GL_RENDERBUFFER, 0);

      glBindFramebuffer(GL_FRAMEBUFFER, name);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D_MULTISAMPLE, texture.Name(), 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER, render_buffer.Name());
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }

  ~FrameBuffer() { glDeleteFramebuffers(1, &name); }

  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer& operator=(const FrameBuffer&) = delete;
  FrameBuffer(FrameBuffer&&) = delete;
  FrameBuffer& operator=(FrameBuffer&&) = delete;

  [[nodiscard]] GLenum CheckStatus() const {
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    GLenum const status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return status;
  }

  [[nodiscard]] GLuint Name() const { return name; }

  [[nodiscard]] GLuint TextureName() const { return texture.Name(); }

 private:
  GLuint name{0};
  Texture texture;
  RenderBuffer render_buffer;
};

// Renders into an off-screen framebuffer and reads the result back as RGBA
// bytes. With multisampling (samples > 1) it renders into a dedicated MSAA
// framebuffer and resolves it into the readable output buffer; without it,
// rendering goes straight into the output buffer and the extra MSAA buffer is
// never allocated.
class RenderToTexture {
 public:
  RenderToTexture(GLsizei width_in, GLsizei height_in, GLsizei samples_in)
      : width(width_in),
        height(height_in),
        multisampled(samples_in > 1),
        output_frame_buffer(width_in, height_in, samples_in, false) {
    if (multisampled) {
      multisample_frame_buffer =
          std::make_unique<FrameBuffer>(width_in, height_in, samples_in, true);
    }

    valid = output_frame_buffer.CheckStatus() == GL_FRAMEBUFFER_COMPLETE &&
            (!multisampled || multisample_frame_buffer->CheckStatus() ==
                                  GL_FRAMEBUFFER_COMPLETE);
  }

  [[nodiscard]] bool Valid() const { return valid; }

  void BeginRender() {
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());

    previous_viewport_width = viewport[2];
    previous_viewport_height = viewport[3];

    const GLuint render_target = multisampled ? multisample_frame_buffer->Name()
                                              : output_frame_buffer.Name();
    glBindFramebuffer(GL_FRAMEBUFFER, render_target);
    glViewport(0, 0, width, height);
  }

  void EndRender() {
    // Resolve the multisampled colour buffer into the single-sample output
    // buffer. Without MSAA we render straight into the output buffer, so no
    // blit is needed.
    if (multisampled) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, multisample_frame_buffer->Name());
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output_frame_buffer.Name());
      glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, previous_viewport_width, previous_viewport_height);
  }

  std::vector<unsigned char> CopyImage() {
    const size_t image_size = static_cast<size_t>(width) *
                              static_cast<size_t>(height) *
                              static_cast<size_t>(bytes_per_pixel);
    image.resize(image_size);

    glBindTexture(GL_TEXTURE_2D, output_frame_buffer.TextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return image;
  }

 private:
  static constexpr GLsizei kBytesPerPixel = 4;

  GLsizei width;
  GLsizei height;
  bool multisampled;

  FrameBuffer output_frame_buffer;
  std::unique_ptr<FrameBuffer> multisample_frame_buffer;

  GLint previous_viewport_width{0};
  GLint previous_viewport_height{0};

  GLsizei bytes_per_pixel{kBytesPerPixel};
  std::vector<unsigned char> image;

  bool valid{false};
};
#endif  // RENDER_TO_TEXTURE_HPP
