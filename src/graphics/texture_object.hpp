#ifndef TEXTURE_OBJECT_HPP
#define TEXTURE_OBJECT_HPP

#include <epoxy/gl.h>

#include <utility>

class Texture {
 public:
  Texture() { glGenTextures(1, &name); }

  ~Texture() {
    if (name != 0) {
      glDeleteTextures(1, &name);
    }
  }

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& other) noexcept : name(std::exchange(other.name, 0)) {}
  Texture& operator=(Texture&& other) noexcept {
    if (this != &other) {
      if (name != 0) {
        glDeleteTextures(1, &name);
      }
      name = std::exchange(other.name, 0);
    }
    return *this;
  }

  [[nodiscard]] GLuint Name() const { return name; }

 private:
  GLuint name{0};
};
#endif  // TEXTURE_OBJECT_HPP
