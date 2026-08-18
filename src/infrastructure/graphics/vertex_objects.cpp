#include "vertex_objects.hpp"

#include <epoxy/gl.h>

#include <utility>

VertexArrayObject::VertexArrayObject() { glCreateVertexArrays(1, &name_); }

VertexArrayObject::~VertexArrayObject() { glDeleteVertexArrays(1, &name_); }

VertexArrayObject::VertexArrayObject(VertexArrayObject&& other) noexcept
    : name_(std::exchange(other.name_, 0)) {}

VertexArrayObject& VertexArrayObject::operator=(
    VertexArrayObject&& other) noexcept {
  if (this != &other) {
    if (name_ != 0) {
      glDeleteVertexArrays(1, &name_);
    }
    name_ = std::exchange(other.name_, 0);
  }
  return *this;
}

void VertexArrayObject::Bind() const { glBindVertexArray(name_); }

void VertexArrayObject::Unbind() { glBindVertexArray(0); }

GLuint VertexArrayObject::Name() const { return name_; }

VertexBufferObject::VertexBufferObject() { glCreateBuffers(1, &name_); }

VertexBufferObject::~VertexBufferObject() { glDeleteBuffers(1, &name_); }

VertexBufferObject::VertexBufferObject(VertexBufferObject&& other) noexcept
    : name_(std::exchange(other.name_, 0)) {}

VertexBufferObject& VertexBufferObject::operator=(
    VertexBufferObject&& other) noexcept {
  if (this != &other) {
    if (name_ != 0) {
      glDeleteBuffers(1, &name_);
    }
    name_ = std::exchange(other.name_, 0);
  }
  return *this;
}

void VertexBufferObject::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, name_); }

void VertexBufferObject::Unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

GLuint VertexBufferObject::Name() const { return name_; }
