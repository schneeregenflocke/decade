#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_INFO_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_INFO_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <string>
#include <vector>

class ShaderInfo {
 public:
  enum class InfoType : std::uint8_t { ActiveAttribute, ActiveUniform, None };

  struct InfoParams {
    GLint location;
    GLint size;
    GLenum type;
  };

  ShaderInfo() = default;

  ShaderInfo(InfoType info_type_value, std::string name_value,
             InfoParams params)
      : info_type(info_type_value),
        name(std::move(name_value)),
        location(params.location),
        size(params.size),
        type(params.type) {
    UpdateTypeInfo();
  }

  void UpdateTypeInfo() {
    constexpr size_t kVec2Components = 2;
    constexpr size_t kVec3Components = 3;
    constexpr size_t kVec4Components = 4;
    constexpr size_t kMat4Components = 16;

    switch (type) {
      case GL_FLOAT_VEC2:
        type_str = "GL_FLOAT_VEC2";
        number = kVec2Components;
        type_size = sizeof(glm::vec2);
        break;
      case GL_FLOAT_VEC3:
        type_str = "GL_FLOAT_VEC3";
        number = kVec3Components;
        type_size = sizeof(glm::vec3);
        break;
      case GL_FLOAT_VEC4:
        type_str = "GL_FLOAT_VEC4";
        number = kVec4Components;
        type_size = sizeof(glm::vec4);
        break;
      case GL_FLOAT_MAT4:
        type_str = "GL_FLOAT_MAT4";
        number = kMat4Components;
        type_size = sizeof(glm::mat4);
        break;
      case GL_SAMPLER_2D:
        type_str = "GL_SAMPLER_2D";
        number = 1;
        type_size = sizeof(GLint);
        break;
      default:
        type_str = "UNKNOWN";
        number = 0;
        type_size = 0;
        break;
    }
  }

  [[nodiscard]] InfoType GetInfoType() const { return info_type; }
  [[nodiscard]] const std::string& GetName() const { return name; }
  [[nodiscard]] GLint GetLocation() const { return location; }
  [[nodiscard]] GLint GetSize() const { return size; }
  [[nodiscard]] GLenum GetType() const { return type; }
  [[nodiscard]] size_t GetNumber() const { return number; }
  [[nodiscard]] size_t GetTypeSize() const { return type_size; }
  [[nodiscard]] const std::string& GetTypeString() const { return type_str; }

  void Print() const {
    std::string info_type_str;

    if (info_type == InfoType::ActiveAttribute) {
      info_type_str = "Active Attribute";
    } else if (info_type == InfoType::ActiveUniform) {
      info_type_str = "Active Uniform";
    }

    std::cout << "Info_Type: " << info_type_str << ", Name: " << name
              << ", Location: " << location << ", Type_String: " << type_str
              << ", Number: " << number << ", Type_Size: " << type_size
              << ", Size: " << size << ", Type: " << std::hex << type
              << std::dec << '\n';
  }

 private:
  InfoType info_type{InfoType::None};
  std::string name;
  GLint location{0};
  GLint size{0};
  GLenum type{0};
  size_t number{0};
  size_t type_size{0};
  std::string type_str;
};

class ShaderInfos {
 public:
  ShaderInfos() = default;

  void SetProgram(GLuint new_program) {
    program = new_program;

    GatherAttributesInfo();
    GatherUniformsInfo();
  }

  [[nodiscard]] const std::vector<ShaderInfo>& GetAttributesInfos() const {
    return attribute_infos;
  }

  [[nodiscard]] int GetNumberAttributes() const {
    GLint num_attribs = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_attribs);
    return num_attribs;
  }

  [[nodiscard]] int GetNumberUniforms() const {
    GLint num_uniforms = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);
    return num_uniforms;
  }

  void PrintAttributesInfo() const {
    for (const auto& shader_info : attribute_infos) {
      shader_info.Print();
    }
  }

  void PrintUniformsInfo() const {
    for (const auto& shader_info : uniform_infos) {
      shader_info.Print();
    }
  }

 private:
  void SortAttributesInfo() {
    std::sort(attribute_infos.begin(), attribute_infos.end(),
              [](const ShaderInfo& left_info, const ShaderInfo& right_info) {
                return left_info.GetLocation() < right_info.GetLocation();
              });
  }

  void SortUniformsInfo() {
    std::sort(uniform_infos.begin(), uniform_infos.end(),
              [](const ShaderInfo& left_info, const ShaderInfo& right_info) {
                return left_info.GetLocation() < right_info.GetLocation();
              });
  }

  void GatherAttributesInfo() {
    const GLint num_attribs = GetNumberAttributes();
    GLint max_attrib_name_length = 0;
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
                   &max_attrib_name_length);

    for (int index = 0; index < num_attribs; ++index) {
      std::vector<char> attrib_name(static_cast<size_t>(max_attrib_name_length),
                                    '\0');
      GLsizei written = 0;
      GLint size = 0;
      GLenum type = 0;
      glGetActiveAttrib(program, static_cast<GLuint>(index),
                        max_attrib_name_length, &written, &size, &type,
                        attrib_name.data());
      const GLint location = glGetAttribLocation(program, attrib_name.data());

      attribute_infos.emplace_back(
          ShaderInfo::InfoType::ActiveAttribute,
          std::string(attrib_name.data(), static_cast<size_t>(written)),
          ShaderInfo::InfoParams{location, size, type});
    }

    SortAttributesInfo();
  }

  void GatherUniformsInfo() {
    const GLint num_uniforms = GetNumberUniforms();
    GLint max_uniforms_name_length = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH,
                   &max_uniforms_name_length);

    for (int index = 0; index < num_uniforms; ++index) {
      std::vector<char> uniform_name(
          static_cast<size_t>(max_uniforms_name_length), '\0');
      GLsizei written = 0;
      GLint size = 0;
      GLenum type = 0;
      glGetActiveUniform(program, static_cast<GLuint>(index),
                         max_uniforms_name_length, &written, &size, &type,
                         uniform_name.data());

      const GLint location = glGetUniformLocation(program, uniform_name.data());

      uniform_infos.emplace_back(
          ShaderInfo::InfoType::ActiveUniform,
          std::string(uniform_name.data(), static_cast<size_t>(written)),
          ShaderInfo::InfoParams{location, size, type});
    }

    SortUniformsInfo();
  }

  GLuint program{0};
  std::vector<ShaderInfo> attribute_infos;
  std::vector<ShaderInfo> uniform_infos;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_INFO_HPP
