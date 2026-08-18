#include "shapes_base.hpp"

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

#include "rect.hpp"
#include "shaders.hpp"
#include "shaders_info.hpp"
#include "vertex_objects.hpp"

Shape::Shape(Shader& shader_in) : shader_(shader_in) { SetUpBuffers(); }

void Shape::Hide() {
  number_vertices_ = 0;
  local_bounds_ = {};
}

void Shape::Draw(const glm::mat4& model) const {
  shader_.UseProgram();
  shader_.SetUniform("model", model);
  vao_.Bind();
  glDrawArrays(GL_TRIANGLES, 0, number_vertices_);
  VertexArrayObject::Unbind();
}

const RectF& Shape::LocalBounds() const { return local_bounds_; }

GLsizei Shape::VertexCount() const { return number_vertices_; }

Shader& Shape::GetShader() const { return shader_; }

const VertexArrayObject& Shape::VaoRef() const { return vao_; }

void Shape::SetLocalBounds(const RectF& bounds) { local_bounds_ = bounds; }

std::size_t Shape::BufferIndexFor(std::string_view attribute_name,
                                  std::size_t vertex_size) const {
  for (std::size_t index = 0; index < attributes_infos_.size(); ++index) {
    const ShaderInfo& attribute_info = attributes_infos_[index];
    if (attribute_info.GetName() != attribute_name) {
      continue;
    }
    if (attribute_info.GetTypeSize() != vertex_size) {
      throw std::runtime_error(
          "shader '" + shader_.GetName() + "' declares attribute '" +
          std::string(attribute_name) + "' as " +
          attribute_info.GetTypeString() + ", which is " +
          std::to_string(attribute_info.GetTypeSize()) + " bytes wide, but " +
          std::to_string(vertex_size) + " bytes arrive per vertex");
    }
    return index;
  }
  throw std::runtime_error("shader '" + shader_.GetName() +
                           "' carries no active attribute '" +
                           std::string(attribute_name) + "'");
}

void Shape::SetUpBuffers() {
  attributes_infos_ = shader_.GetShaderAttributesInfos();

  vbos_.resize(attributes_infos_.size());

  for (std::size_t index = 0; index < attributes_infos_.size(); ++index) {
    const ShaderInfo& attribute_info = attributes_infos_[index];

    if (!attribute_info.IsFloatVector()) {
      throw std::runtime_error(
          "shader '" + shader_.GetName() + "' declares attribute '" +
          attribute_info.GetName() + "' as " + attribute_info.GetTypeString() +
          "; a shape feeds float vectors of one to four components alone");
    }

    // Each attribute owns a buffer of its own, tightly packed — hence the
    // element size as the stride and no offset anywhere. The three calls split
    // what glVertexAttribPointer once welded together: what an element looks
    // like, which binding point feeds the attribute, and what sits on that
    // point. None of them reads a bound buffer, so nothing gets bound here.
    const GLuint vertex_array = vao_.Name();
    const auto location = static_cast<GLuint>(attribute_info.GetLocation());
    const auto binding_index = static_cast<GLuint>(index);

    glVertexArrayAttribFormat(vertex_array, location,
                              static_cast<GLint>(attribute_info.GetNumber()),
                              GL_FLOAT, GL_FALSE, 0);

    glVertexArrayAttribBinding(vertex_array, location, binding_index);

    glVertexArrayVertexBuffer(
        vertex_array, binding_index, vbos_[index].Name(), 0,
        static_cast<GLsizei>(attribute_info.GetTypeSize()));

    glEnableVertexArrayAttrib(vertex_array, location);
  }
}
