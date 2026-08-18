#include "shapes_base.hpp"

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <utility>

#include "rect.hpp"
#include "shaders.hpp"
#include "vertex_objects.hpp"

Shape::Shape(Shader& shader_in) : shader_(shader_in) { SetUpBuffers(); }

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

void Shape::SetUpBuffers() {
  attributes_infos_ = shader_.GetShaderAttributesInfos();

  vao_.Bind();

  vbos_.resize(attributes_infos_.size());

  for (size_t index = 0; index < attributes_infos_.size(); ++index) {
    const auto& attribute_info = attributes_infos_[index];

    vbos_[index].Bind();

    const auto attribute_location =
        static_cast<GLuint>(attribute_info.GetLocation());

    glVertexAttribFormat(attribute_location,
                         static_cast<GLint>(attribute_info.GetNumber()),
                         GL_FLOAT, GL_FALSE, 0);

    const auto binding_index = static_cast<GLuint>(index);

    glVertexAttribBinding(attribute_location, binding_index);

    glBindVertexBuffer(binding_index, vbos_[index].Name(), 0,
                       static_cast<GLsizei>(attribute_info.GetTypeSize()));

    glEnableVertexAttribArray(attribute_location);

    VertexBufferObject::Unbind();
  }

  VertexArrayObject::Unbind();
}
