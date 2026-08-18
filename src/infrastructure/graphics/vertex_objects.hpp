#ifndef VERTEX_OBJECTS_HPP
#define VERTEX_OBJECTS_HPP

#include <epoxy/gl.h>

// RAII around the two GL object names a vertex array needs.
class VertexArrayObject {
 public:
  VertexArrayObject();

  ~VertexArrayObject();

  VertexArrayObject(const VertexArrayObject&) = delete;
  VertexArrayObject& operator=(const VertexArrayObject&) = delete;

  VertexArrayObject(VertexArrayObject&& other) noexcept;
  VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;

  void Bind() const;

  static void Unbind();

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};

class VertexBufferObject {
 public:
  VertexBufferObject();

  ~VertexBufferObject();

  VertexBufferObject(const VertexBufferObject&) = delete;
  VertexBufferObject& operator=(const VertexBufferObject&) = delete;

  VertexBufferObject(VertexBufferObject&& other) noexcept;
  VertexBufferObject& operator=(VertexBufferObject&& other) noexcept;

  void Bind() const;

  static void Unbind();

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};

#endif  // VERTEX_OBJECTS_HPP
