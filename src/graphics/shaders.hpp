#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <epoxy/gl.h>

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Resource.h"
#include "shaders_info.hpp"

class Shader {
 public:
  struct ShaderSources {
    std::string vertex;
    std::string fragment;
  };

  explicit Shader(const ShaderSources& sources, std::string name_in)
      : name_(std::move(name_in)) {
    CompileProgram(sources);

    shader_info_.SetProgram(program_);
  }

  [[nodiscard]] GLuint GetProgram() const { return program_; }

  [[nodiscard]] std::string get_name() const { return name_; }

  void UseProgram() const { glUseProgram(program_); }

  void SetUniform(const std::string& uniform_name,
                  const glm::mat4& matrix) const {
    auto location = glGetUniformLocation(program_, uniform_name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
  }

  void SetUniform(const std::string& uniform_name,
                  const glm::vec4& vector) const {
    auto location = glGetUniformLocation(program_, uniform_name.c_str());
    glUniform4fv(location, 1, glm::value_ptr(vector));
    // glProgramUniform
  }

  void PrintShaderInfo() const {
    std::cout << "Shader: " << name_ << ", Number of attributes: "
              << shader_info_.GetNumberAttributes()
              << ", Number of uniforms: " << shader_info_.GetNumberUniforms()
              << '\n';

    shader_info_.PrintAttributesInfo();
    shader_info_.PrintUniformsInfo();
  }

  [[nodiscard]] const ShaderInfos& GetShaderInfo() const {
    return shader_info_;
  }

  [[nodiscard]] const std::vector<ShaderInfo>& GetShaderAttributesInfos()
      const {
    return shader_info_.GetAttributesInfos();
  }

 private:
  struct ShaderHandles {
    GLuint vertex;
    GLuint fragment;
  };

  void CompileProgram(const ShaderSources& sources) {
    const ShaderHandles handles{
        .vertex = CompileShader(sources.vertex, GL_VERTEX_SHADER),
        .fragment = CompileShader(sources.fragment, GL_FRAGMENT_SHADER)};

    LinkShaders(handles);
  }

  void LinkShaders(const ShaderHandles& handles) {
    program_ = glCreateProgram();

    glAttachShader(program_, handles.vertex);
    glAttachShader(program_, handles.fragment);

    glLinkProgram(program_);

    glValidateProgram(program_);

    GLint status = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &status);
    std::cout << "GL_LINK_STATUS: " << program_ << " " << std::boolalpha
              << static_cast<bool>(status) << std::noboolalpha << '\n';

    GLint info_length = 0;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &info_length);
    if (info_length > 0) {
      std::vector<char> info_log(static_cast<size_t>(info_length));
      glGetProgramInfoLog(program_, info_length, nullptr, info_log.data());
      std::cout << info_log.data();
    }

    glDetachShader(program_, handles.vertex);
    glDetachShader(program_, handles.fragment);
    glDeleteShader(handles.vertex);
    glDeleteShader(handles.fragment);
  }

  static GLuint CompileShader(const std::string& source, GLenum shader_type) {
    const GLuint shader = glCreateShader(shader_type);
    const std::array<const char*, 1> source_strings = {source.c_str()};
    glShaderSource(shader, static_cast<GLsizei>(source_strings.size()),
                   source_strings.data(), nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    std::cout << "GL_COMPILE_STATUS: " << shader << " " << std::boolalpha
              << static_cast<bool>(status) << std::noboolalpha << '\n';

    GLint info_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
    if (info_length > 0) {
      std::vector<char> info_log(static_cast<size_t>(info_length));
      glGetShaderInfoLog(shader, info_length, nullptr, info_log.data());
      std::cout << info_log.data();
    }

    return shader;
  }

  GLuint program_{0};
  std::string name_;
  ShaderInfos shader_info_;
};

class Shaders {
 public:
  Shaders() {
    auto simple_vertex_shader_resource =
        LOAD_RESOURCE(shader_simple_vertex_shader);
    auto simple_fragment_shader_resource =
        LOAD_RESOURCE(shader_simple_fragment_shader);
    auto rectangles_vertex_shader_resource =
        LOAD_RESOURCE(shader_rectangles_vertex_shader);
    auto rectangles_fragment_shader_resource =
        LOAD_RESOURCE(shader_rectangles_fragment_shader);
    auto font_vertex_shader_resource = LOAD_RESOURCE(shader_font_vertex_shader);
    auto font_fragment_shader_resource =
        LOAD_RESOURCE(shader_font_fragment_shader);
    // auto phong_vertex_shader_resource =
    // LOAD_RESOURCE(shader_phong_vertex_shader); auto
    // phong_fragment_shader_resource =
    // LOAD_RESOURCE(shader_phong_fragment_shader);

    shaders_.emplace_back(
        Shader::ShaderSources{
            .vertex = simple_vertex_shader_resource.toString(),
            .fragment = simple_fragment_shader_resource.toString()},
        "Simple Shader");
    shaders_.emplace_back(
        Shader::ShaderSources{
            .vertex = rectangles_vertex_shader_resource.toString(),
            .fragment = rectangles_fragment_shader_resource.toString()},
        "Rectangles Shader");
    shaders_.emplace_back(
        Shader::ShaderSources{
            .vertex = font_vertex_shader_resource.toString(),
            .fragment = font_fragment_shader_resource.toString()},
        "Font Shader");
    // shaders.push_back(Shader(phong_vertex_shader_resource.toString(),
    // phong_fragment_shader_resource.toString(), std::string("Phong Shader")));

    PrintInfo();
  }

  void PrintInfo() const {
    for (const auto& shader : shaders_) {
      shader.PrintShaderInfo();
    }
  }

  Shader* GetShader(size_t index) {
    if (index < shaders_.size()) {
      return &shaders_[index];
    }
    throw std::invalid_argument("Shader index out of bounds");
  }

  std::optional<Shader*> search_shader(const std::string& search_name) {
    for (auto& shader : shaders_) {
      if (shader.get_name() == search_name) {
        return std::optional<Shader*>{&shader};
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] size_t GetNumberShaders() const { return shaders_.size(); }

 private:
  std::vector<Shader> shaders_;
};
#endif  // SHADERS_HPP
