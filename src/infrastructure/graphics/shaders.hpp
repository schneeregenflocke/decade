#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../common/debug_log.hpp"
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

  [[nodiscard]] std::string GetName() const { return name_; }

  void UseProgram() const { glUseProgram(program_); }

  void SetUniform(const std::string& uniform_name,
                  const glm::mat4& matrix) const {
    glUniformMatrix4fv(UniformLocation(uniform_name), 1, GL_FALSE,
                       glm::value_ptr(matrix));
  }

  void SetUniform(const std::string& uniform_name,
                  const glm::vec4& vector) const {
    glUniform4fv(UniformLocation(uniform_name), 1, glm::value_ptr(vector));
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

  // Uniform locations are stable after linking; the cache spares the
  // glGetUniformLocation string lookup that would otherwise fall due per shape
  // and frame.
  GLint UniformLocation(const std::string& uniform_name) const {
    auto found = uniform_locations_.find(uniform_name);
    if (found == uniform_locations_.end()) {
      found = uniform_locations_
                  .emplace(uniform_name,
                           glGetUniformLocation(program_, uniform_name.c_str()))
                  .first;
    }
    return found->second;
  }

  void CompileProgram(const ShaderSources& sources) {
    // The stage travels into the message: the driver reports a line and a
    // column but no file, so without it a failure names no source at all.
    const ShaderHandles handles{
        .vertex = CompileShader(sources.vertex, GL_VERTEX_SHADER,
                                name_ + " vertex shader"),
        .fragment = CompileShader(sources.fragment, GL_FRAGMENT_SHADER,
                                  name_ + " fragment shader")};

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
    if (decade_debug::LogEnabled()) {
      std::cout << "GL_LINK_STATUS: " << program_ << " " << std::boolalpha
                << static_cast<bool>(status) << std::noboolalpha << '\n';
    }

    // The driver's log is not itself a fault — on a successful link it holds
    // notes or nothing, and stays behind the flag then. On a failed one it
    // holds the reason, and that is the only thing anybody can act on, so it
    // travels with the exception instead of scrolling past.
    GLint info_length = 0;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &info_length);
    std::string info_log(static_cast<size_t>(std::max(info_length, 1)) - 1,
                         '\0');
    if (info_length > 0) {
      glGetProgramInfoLog(program_, info_length, nullptr, info_log.data());
    }

    if (status == GL_FALSE) {
      throw std::runtime_error("linking shader program '" + name_ +
                               "' failed: " + info_log);
    }
    if (!info_log.empty() && decade_debug::LogEnabled()) {
      std::cout << info_log;
    }

    glDetachShader(program_, handles.vertex);
    glDetachShader(program_, handles.fragment);
    glDeleteShader(handles.vertex);
    glDeleteShader(handles.fragment);
  }

  static GLuint CompileShader(const std::string& source, GLenum shader_type,
                              const std::string& label) {
    const GLuint shader = glCreateShader(shader_type);
    const std::array<const char*, 1> source_strings = {source.c_str()};
    glShaderSource(shader, static_cast<GLsizei>(source_strings.size()),
                   source_strings.data(), nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (decade_debug::LogEnabled()) {
      std::cout << "GL_COMPILE_STATUS: " << label << ' ' << std::boolalpha
                << static_cast<bool>(status) << std::noboolalpha << '\n';
    }

    // Same split as when linking.
    GLint info_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_length);
    std::string info_log(static_cast<size_t>(std::max(info_length, 1)) - 1,
                         '\0');
    if (info_length > 0) {
      glGetShaderInfoLog(shader, info_length, nullptr, info_log.data());
    }

    if (status == GL_FALSE) {
      glDeleteShader(shader);
      throw std::runtime_error("compiling " + label + " failed: " + info_log);
    }
    if (!info_log.empty() && decade_debug::LogEnabled()) {
      std::cout << info_log;
    }

    return shader;
  }

  GLuint program_{0};
  std::string name_;
  ShaderInfos shader_info_;
  mutable std::unordered_map<std::string, GLint> uniform_locations_;
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

    // The Lambert shaders (diffuse lighting per fragment) are embedded as a
    // resource but deliberately not loaded: they belong to the 3D path, which
    // the calendar rendering does not use.

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
    PrintInfo();
  }

  // Every shader with its attributes and uniforms — diagnosis, so it hangs on
  // --debug-log. One guard for the whole chain: PrintShaderInfo and the
  // ShaderInfo printers below it have no other caller.
  void PrintInfo() const {
    if (!decade_debug::LogEnabled()) {
      return;
    }
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

  std::optional<Shader*> SearchShader(const std::string& search_name) {
    for (auto& shader : shaders_) {
      if (shader.GetName() == search_name) {
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
