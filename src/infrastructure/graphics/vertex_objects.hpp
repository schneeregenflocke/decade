#ifndef VERTEX_OBJECTS_HPP
#define VERTEX_OBJECTS_HPP

#include <epoxy/gl.h>

// RAII around the two GL object names a vertex array needs. Both get created
// the direct-state-access way (`glCreate*` rather than `glGen*`), which is what
// lets every call configuring them take the name as a parameter.
class VertexArrayObject {
 public:
  VertexArrayObject();

  ~VertexArrayObject();

  VertexArrayObject(const VertexArrayObject&) = delete;
  VertexArrayObject& operator=(const VertexArrayObject&) = delete;

  VertexArrayObject(VertexArrayObject&& other) noexcept;
  VertexArrayObject& operator=(VertexArrayObject&& other) noexcept;

  // Drawing is the one thing that still reads from the binding point:
  // glDrawArrays sources its attributes from the bound vertex array.
  void Bind() const;

  static void Unbind();

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};

// No Bind of its own: the format, the vertex buffer binding and the upload all
// name the buffer directly, so a GL_ARRAY_BUFFER binding would be ceremony that
// changes nothing.
class VertexBufferObject {
 public:
  VertexBufferObject();

  ~VertexBufferObject();

  VertexBufferObject(const VertexBufferObject&) = delete;
  VertexBufferObject& operator=(const VertexBufferObject&) = delete;

  VertexBufferObject(VertexBufferObject&& other) noexcept;
  VertexBufferObject& operator=(VertexBufferObject&& other) noexcept;

  [[nodiscard]] GLuint Name() const;

 private:
  GLuint name_{0};
};

#endif  // VERTEX_OBJECTS_HPP
