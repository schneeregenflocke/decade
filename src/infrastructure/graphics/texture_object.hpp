#ifndef TEXTURE_OBJECT_HPP
#define TEXTURE_OBJECT_HPP

#include <epoxy/gl.h>

#include <utility>

class Texture {
 public:
  Texture() { glGenTextures(1, &name_); }

  ~Texture() {
    if (name_ != 0) {
      glDeleteTextures(1, &name_);
    }
  }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& other) noexcept : name_(std::exchange(other.name_, 0)) {}
  Texture& operator=(Texture&& other) noexcept {
    if (this != &other) {
      if (name_ != 0) {
        glDeleteTextures(1, &name_);
      }
      name_ = std::exchange(other.name_, 0);
    }
    return *this;
  }

  [[nodiscard]] GLuint Name() const { return name_; }

 private:
  GLuint name_{0};
};
#endif  // TEXTURE_OBJECT_HPP
