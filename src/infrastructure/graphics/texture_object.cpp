#include "texture_object.hpp"

#include <epoxy/gl.h>

#include <utility>

Texture::Texture() { glGenTextures(1, &name_); }

Texture::~Texture() {
  if (name_ != 0) {
    glDeleteTextures(1, &name_);
  }
}

Texture::Texture(Texture&& other) noexcept
    : name_(std::exchange(other.name_, 0)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
  if (this != &other) {
    if (name_ != 0) {
      glDeleteTextures(1, &name_);
    }
    name_ = std::exchange(other.name_, 0);
  }
  return *this;
}

GLuint Texture::Name() const { return name_; }
