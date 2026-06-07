#ifndef RENDER_TO_TEXTURE_HPP
#define RENDER_TO_TEXTURE_HPP

#include <epoxy/gl.h>

#include <array>
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
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

class RenderToTexture {
 public:
  RenderToTexture(GLsizei width_in, GLsizei height_in, GLsizei samples_in)
      : width(width_in),
        height(height_in),
        frame_buffer(width_in, height_in, samples_in, false),
        frame_buffer_msaa(width_in, height_in, samples_in, true) {
    if (frame_buffer.CheckStatus() == GL_FRAMEBUFFER_COMPLETE &&
        frame_buffer_msaa.CheckStatus() == GL_FRAMEBUFFER_COMPLETE) {
      valid = true;
    }
  }

  [[nodiscard]] bool Valid() const { return valid; }

  void BeginRender() {
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());

    restore_width = viewport[2];
    restore_height = viewport[3];

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_msaa.Name());
    glViewport(0, 0, width, height);
  }

  void EndRender() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_msaa.Name());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer.Name());
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, restore_width, restore_height);
  }

  std::vector<unsigned char> CopyImage() {
    const size_t image_size = static_cast<size_t>(width) *
                              static_cast<size_t>(height) *
                              static_cast<size_t>(pixel_size);
    image.resize(image_size);

    glBindTexture(GL_TEXTURE_2D, frame_buffer.TextureName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return image;
  }

 private:
  static constexpr GLsizei kBytesPerPixel = 4;

  GLsizei width;
  GLsizei height;

  FrameBuffer frame_buffer;
  FrameBuffer frame_buffer_msaa;

  GLint restore_width{0};
  GLint restore_height{0};

  GLsizei pixel_size{kBytesPerPixel};
  std::vector<unsigned char> image;

  bool valid{false};
};
#endif  // RENDER_TO_TEXTURE_HPP
