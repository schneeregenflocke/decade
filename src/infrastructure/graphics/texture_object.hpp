#ifndef TEXTURE_OBJECT_HPP
#define TEXTURE_OBJECT_HPP

#include <epoxy/gl.h>

class Texture {
 public:
  Texture();

  ~Texture();

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};
#endif  // TEXTURE_OBJECT_HPP
