#include "shaders.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../common/debug_log.hpp"
#include "../../common/embedded_resources.hpp"
#include "mvp_matrices.hpp"
#include "shaders_info.hpp"

Shader::Shader(const ShaderSources& sources, std::string name_in)
    : name_(std::move(name_in)) {
  CompileProgram(sources);

  shader_info_.SetProgram(program_);
}

GLuint Shader::GetProgram() const { return program_; }

std::string Shader::GetName() const { return name_; }

void Shader::UseProgram() const { glUseProgram(program_); }

void Shader::SetUniform(const std::string& uniform_name,
                        const glm::mat4& matrix) const {
  glUniformMatrix4fv(UniformLocation(uniform_name), 1, GL_FALSE,
                     glm::value_ptr(matrix));
}

void Shader::SetUniform(const std::string& uniform_name,
                        const glm::vec4& vector) const {
  glUniform4fv(UniformLocation(uniform_name), 1, glm::value_ptr(vector));
}

void Shader::PrintShaderInfo() const {
  std::cout << "Shader: " << name_
            << ", Number of attributes: " << shader_info_.GetNumberAttributes()
            << ", Number of uniforms: " << shader_info_.GetNumberUniforms()
            << '\n';

  shader_info_.PrintAttributesInfo();
  shader_info_.PrintUniformsInfo();
}

const ShaderInfos& Shader::GetShaderInfo() const { return shader_info_; }

const std::vector<ShaderInfo>& Shader::GetShaderAttributesInfos() const {
  return shader_info_.GetAttributesInfos();
}

GLint Shader::UniformLocation(const std::string& uniform_name) const {
  auto found = uniform_locations_.find(uniform_name);
  if (found == uniform_locations_.end()) {
    found = uniform_locations_
                .emplace(uniform_name,
                         glGetUniformLocation(program_, uniform_name.c_str()))
                .first;
  }
  return found->second;
}

void Shader::CompileProgram(const ShaderSources& sources) {
  // The stage travels into the message: the driver reports a line and a
  // column but no file, so without it a failure names no source at all.
  const ShaderHandles handles{
      .vertex = CompileShader(sources.vertex, GL_VERTEX_SHADER,
                              name_ + " vertex shader"),
      .fragment = CompileShader(sources.fragment, GL_FRAGMENT_SHADER,
                                name_ + " fragment shader")};

  LinkShaders(handles);
}

void Shader::LinkShaders(const ShaderHandles& handles) {
  program_ = glCreateProgram();

  glAttachShader(program_, handles.vertex);
  glAttachShader(program_, handles.fragment);

  glLinkProgram(program_);

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
  std::string info_log(static_cast<size_t>(std::max(info_length, 1)) - 1, '\0');
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

GLuint Shader::CompileShader(const std::string& source, GLenum shader_type,
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
  std::string info_log(static_cast<size_t>(std::max(info_length, 1)) - 1, '\0');
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

Shaders::Shaders() {
  // The Lambert shaders (diffuse lighting per fragment) used to be embedded
  // beside these and never loaded — they belong to the 3D path the calendar
  // does not use. With #embed, a file nobody names simply does not travel
  // into the binary any more, so they are gone rather than merely unused.
  shaders_.emplace_back(
      Shader::ShaderSources{
          .vertex = std::string(resources::kSimpleVertexShader),
          .fragment = std::string(resources::kSimpleFragmentShader)},
      "Simple Shader");
  shaders_.emplace_back(
      Shader::ShaderSources{
          .vertex = std::string(resources::kRectanglesVertexShader),
          .fragment = std::string(resources::kRectanglesFragmentShader)},
      "Rectangles Shader");
  shaders_.emplace_back(
      Shader::ShaderSources{
          .vertex = std::string(resources::kFontVertexShader),
          .fragment = std::string(resources::kFontFragmentShader)},
      "Font Shader");
  PrintInfo();
}

void Shaders::PrintInfo() const {
  if (!decade_debug::LogEnabled()) {
    return;
  }
  for (const auto& shader : shaders_) {
    shader.PrintShaderInfo();
  }
}

void Shaders::SetCameraUniforms(const MVP& mvp) {
  for (const auto& shader : shaders_) {
    shader.UseProgram();
    shader.SetUniform("projection", mvp.GetProjection());
    shader.SetUniform("view", mvp.GetView());
  }
}

std::optional<std::reference_wrapper<Shader>> Shaders::SearchShader(
    const std::string& search_name) {
  for (auto& shader : shaders_) {
    if (shader.GetName() == search_name) {
      return std::ref(shader);
    }
  }
  return std::nullopt;
}
