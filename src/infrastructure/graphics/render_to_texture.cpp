#include "render_to_texture.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

ScopedFramebufferBinding::ScopedFramebufferBinding() {
  GLint bound = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
  previous_ = static_cast<GLuint>(bound);
}

ScopedFramebufferBinding::~ScopedFramebufferBinding() {
  glBindFramebuffer(GL_FRAMEBUFFER, previous_);
}

RenderBuffer::RenderBuffer() { glGenRenderbuffers(1, &name_); }

RenderBuffer::~RenderBuffer() { glDeleteRenderbuffers(1, &name_); }

GLuint RenderBuffer::Name() const { return name_; }

FrameBuffer::FrameBuffer(GLsizei width, GLsizei height, GLsizei samples,
                         bool msaa) {
  const ScopedFramebufferBinding restore_binding;
  glGenFramebuffers(1, &name_);

  if (!msaa) {
    glBindTexture(GL_TEXTURE_2D, texture_.Name());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_.Name());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, name_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, render_buffer_.Name());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_.Name(), 0);
  }

  if (msaa) {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture_.Name());
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width,
                            height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_.Name());
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                                     GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, name_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D_MULTISAMPLE, texture_.Name(), 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, render_buffer_.Name());
  }
}

FrameBuffer::~FrameBuffer() { glDeleteFramebuffers(1, &name_); }

GLenum FrameBuffer::CheckStatus() const {
  const ScopedFramebufferBinding restore_binding;
  glBindFramebuffer(GL_FRAMEBUFFER, name_);
  return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

GLuint FrameBuffer::Name() const { return name_; }

GLuint FrameBuffer::TextureName() const { return texture_.Name(); }

GLsizei MaxUsableSamples() {
  GLint max_samples = 0;
  GLint max_color_samples = 0;
  GLint max_depth_samples = 0;
  glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_samples);
  glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_samples);
  return std::min({max_samples, max_color_samples, max_depth_samples});
}

RenderToTexture::RenderToTexture(GLsizei width_in, GLsizei height_in,
                                 GLsizei samples_in)
    : width_(width_in),
      height_(height_in),
      samples_(std::min(samples_in, MaxUsableSamples())),
      multisampled_(samples_ > 1),
      output_frame_buffer_(width_in, height_in, samples_, false) {
  if (multisampled_) {
    multisample_frame_buffer_ =
        std::make_unique<FrameBuffer>(width_in, height_in, samples_, true);
    if (multisample_frame_buffer_->CheckStatus() != GL_FRAMEBUFFER_COMPLETE) {
      multisample_frame_buffer_.reset();
      multisampled_ = false;
    }
  }

  valid_ = output_frame_buffer_.CheckStatus() == GL_FRAMEBUFFER_COMPLETE;
}

bool RenderToTexture::Valid() const { return valid_; }

GLsizei RenderToTexture::Samples() const {
  return multisampled_ ? samples_ : 1;
}

void RenderToTexture::BeginRender() {
  std::array<GLint, 4> viewport{};
  glGetIntegerv(GL_VIEWPORT, viewport.data());

  previous_viewport_width_ = viewport[2];
  previous_viewport_height_ = viewport[3];

  GLint bound_framebuffer = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound_framebuffer);
  previous_framebuffer_ = static_cast<GLuint>(bound_framebuffer);

  const GLuint render_target = multisampled_ ? multisample_frame_buffer_->Name()
                                             : output_frame_buffer_.Name();
  glBindFramebuffer(GL_FRAMEBUFFER, render_target);
  glViewport(0, 0, width_, height_);
}

void RenderToTexture::EndRender() {
  // Resolve the multisampled colour buffer into the single-sample output
  // buffer. Without MSAA we render straight into the output buffer, so no
  // blit is needed.
  if (multisampled_) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisample_frame_buffer_->Name());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output_frame_buffer_.Name());
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer_);

  glViewport(0, 0, previous_viewport_width_, previous_viewport_height_);
}

std::vector<unsigned char> RenderToTexture::CopyImage() {
  const size_t image_size = static_cast<size_t>(width_) *
                            static_cast<size_t>(height_) *
                            static_cast<size_t>(bytes_per_pixel_);
  image_.resize(image_size);

  glBindTexture(GL_TEXTURE_2D, output_frame_buffer_.TextureName());
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  return image_;
}
