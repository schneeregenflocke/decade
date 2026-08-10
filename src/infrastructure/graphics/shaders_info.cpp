#include "shaders_info.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

ShaderInfo::ShaderInfo(InfoType info_type_value, std::string name_value,
                       InfoParams params)
    : info_type_(info_type_value),
      name_(std::move(name_value)),
      location_(params.location),
      size_(params.size),
      type_(params.type) {
  UpdateTypeInfo();
}

void ShaderInfo::UpdateTypeInfo() {
  constexpr size_t kVec2Components = 2;
  constexpr size_t kVec3Components = 3;
  constexpr size_t kVec4Components = 4;
  constexpr size_t kMat4Components = 16;

  switch (type_) {
    case GL_FLOAT_VEC2:
      type_str_ = "GL_FLOAT_VEC2";
      number_ = kVec2Components;
      type_size_ = sizeof(glm::vec2);
      break;
    case GL_FLOAT_VEC3:
      type_str_ = "GL_FLOAT_VEC3";
      number_ = kVec3Components;
      type_size_ = sizeof(glm::vec3);
      break;
    case GL_FLOAT_VEC4:
      type_str_ = "GL_FLOAT_VEC4";
      number_ = kVec4Components;
      type_size_ = sizeof(glm::vec4);
      break;
    case GL_FLOAT_MAT4:
      type_str_ = "GL_FLOAT_MAT4";
      number_ = kMat4Components;
      type_size_ = sizeof(glm::mat4);
      break;
    case GL_SAMPLER_2D:
      type_str_ = "GL_SAMPLER_2D";
      number_ = 1;
      type_size_ = sizeof(GLint);
      break;
    default:
      type_str_ = "UNKNOWN";
      number_ = 0;
      type_size_ = 0;
      break;
  }
}

ShaderInfo::InfoType ShaderInfo::GetInfoType() const { return info_type_; }

const std::string& ShaderInfo::GetName() const { return name_; }

GLint ShaderInfo::GetLocation() const { return location_; }

GLint ShaderInfo::GetSize() const { return size_; }

GLenum ShaderInfo::GetType() const { return type_; }

size_t ShaderInfo::GetNumber() const { return number_; }

size_t ShaderInfo::GetTypeSize() const { return type_size_; }

const std::string& ShaderInfo::GetTypeString() const { return type_str_; }

void ShaderInfo::Print() const {
  std::string info_type_str;

  if (info_type_ == InfoType::ActiveAttribute) {
    info_type_str = "Active Attribute";
  } else if (info_type_ == InfoType::ActiveUniform) {
    info_type_str = "Active Uniform";
  }

  std::cout << "Info_Type: " << info_type_str << ", Name: " << name_
            << ", Location: " << location_ << ", Type_String: " << type_str_
            << ", Number: " << number_ << ", Type_Size: " << type_size_
            << ", Size: " << size_ << ", Type: " << std::hex << type_
            << std::dec << '\n';
}

void ShaderInfos::SetProgram(GLuint new_program) {
  program_ = new_program;

  GatherAttributesInfo();
  GatherUniformsInfo();
}

const std::vector<ShaderInfo>& ShaderInfos::GetAttributesInfos() const {
  return attribute_infos_;
}

int ShaderInfos::GetNumberAttributes() const {
  GLint num_attribs = 0;
  glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTES, &num_attribs);
  return num_attribs;
}

int ShaderInfos::GetNumberUniforms() const {
  GLint num_uniforms = 0;
  glGetProgramiv(program_, GL_ACTIVE_UNIFORMS, &num_uniforms);
  return num_uniforms;
}

void ShaderInfos::PrintAttributesInfo() const {
  for (const auto& shader_info : attribute_infos_) {
    shader_info.Print();
  }
}

void ShaderInfos::PrintUniformsInfo() const {
  for (const auto& shader_info : uniform_infos_) {
    shader_info.Print();
  }
}

void ShaderInfos::SortAttributesInfo() {
  std::ranges::sort(attribute_infos_, [](const ShaderInfo& left_info,
                                         const ShaderInfo& right_info) {
    return left_info.GetLocation() < right_info.GetLocation();
  });
}

void ShaderInfos::SortUniformsInfo() {
  std::ranges::sort(uniform_infos_, [](const ShaderInfo& left_info,
                                       const ShaderInfo& right_info) {
    return left_info.GetLocation() < right_info.GetLocation();
  });
}

void ShaderInfos::GatherAttributesInfo() {
  const GLint num_attribs = GetNumberAttributes();
  GLint max_attrib_name_length = 0;
  glGetProgramiv(program_, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                 &max_attrib_name_length);

  for (int index = 0; index < num_attribs; ++index) {
    std::vector<char> attrib_name(static_cast<size_t>(max_attrib_name_length),
                                  '\0');
    GLsizei written = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveAttrib(program_, static_cast<GLuint>(index),
                      max_attrib_name_length, &written, &size, &type,
                      attrib_name.data());
    const GLint location = glGetAttribLocation(program_, attrib_name.data());

    attribute_infos_.emplace_back(
        ShaderInfo::InfoType::ActiveAttribute,
        std::string(attrib_name.data(), static_cast<size_t>(written)),
        ShaderInfo::InfoParams{
            .location = location, .size = size, .type = type});
  }

  SortAttributesInfo();
}

void ShaderInfos::GatherUniformsInfo() {
  const GLint num_uniforms = GetNumberUniforms();
  GLint max_uniforms_name_length = 0;
  glGetProgramiv(program_, GL_ACTIVE_UNIFORM_MAX_LENGTH,
                 &max_uniforms_name_length);

  for (int index = 0; index < num_uniforms; ++index) {
    std::vector<char> uniform_name(
        static_cast<size_t>(max_uniforms_name_length), '\0');
    GLsizei written = 0;
    GLint size = 0;
    GLenum type = 0;
    glGetActiveUniform(program_, static_cast<GLuint>(index),
                       max_uniforms_name_length, &written, &size, &type,
                       uniform_name.data());

    const GLint location = glGetUniformLocation(program_, uniform_name.data());

    uniform_infos_.emplace_back(
        ShaderInfo::InfoType::ActiveUniform,
        std::string(uniform_name.data(), static_cast<size_t>(written)),
        ShaderInfo::InfoParams{
            .location = location, .size = size, .type = type});
  }

  SortUniformsInfo();
}
