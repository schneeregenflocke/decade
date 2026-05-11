#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_HPP

#include <array>
#include <epoxy/gl.h>
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

  explicit Shader(const ShaderSources &sources, std::string name_in)
      : name(std::move(name_in))
  {
    CompileProgram(sources);

    shader_info.SetProgram(program);
  }

  [[nodiscard]] GLuint GetProgram() const { return program; }

  [[nodiscard]] std::string get_name() const { return name; }

  void UseProgram() const { glUseProgram(program); }

  void SetUniform(const std::string &uniform_name, const glm::mat4 &matrix) const
  {
    auto location = glGetUniformLocation(program, uniform_name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
  }

  void SetUniform(const std::string &uniform_name, const glm::vec4 &vector) const
  {
    auto location = glGetUniformLocation(program, uniform_name.c_str());
    glUniform4fv(location, 1, glm::value_ptr(vector));
    // glProgramUniform
  }

  void PrintShaderInfo() const
  {
    std::cout << "Shader: " << name
              << ", Number of attributes: " << shader_info.GetNumberAttributes()
              << ", Number of uniforms: " << shader_info.GetNumberUniforms() << '\n';

    shader_info.PrintAttributesInfo();
    shader_info.PrintUniformsInfo();
  }

  [[nodiscard]] const ShaderInfos &GetShaderInfo() const { return shader_info; }

  [[nodiscard]] const std::vector<ShaderInfo> &GetShaderAttributesInfos() const
  {
    return shader_info.GetAttributesInfos();
  }

private:
  struct ShaderHandles {
    GLuint vertex;
    GLuint fragment;
  };

  void CompileProgram(const ShaderSources &sources)
  {
    const ShaderHandles handles{
        CompileShader(sources.vertex, GL_VERTEX_SHADER),
        CompileShader(sources.fragment, GL_FRAGMENT_SHADER)};

    LinkShaders(handles);
  }

  void LinkShaders(const ShaderHandles &handles)
  {
    program = glCreateProgram();

    glAttachShader(program, handles.vertex);
    glAttachShader(program, handles.fragment);

    glLinkProgram(program);

    glValidateProgram(program);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    std::cout << "GL_LINK_STATUS: " << program << " " << std::boolalpha << static_cast<bool>(status)
              << std::noboolalpha << '\n';

    GLint info_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_length);
    if (info_length > 0) {
      std::vector<char> info_log(static_cast<size_t>(info_length));
      glGetProgramInfoLog(program, info_length, nullptr, info_log.data());
      std::cout << info_log.data();
    }

    glDetachShader(program, handles.vertex);
    glDetachShader(program, handles.fragment);
    glDeleteShader(handles.vertex);
    glDeleteShader(handles.fragment);
  }

  static GLuint CompileShader(const std::string &source, GLenum shader_type)
  {
    const GLuint shader = glCreateShader(shader_type);
    const std::array<const char *, 1> source_strings = {source.c_str()};
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

  GLuint program{0};
  std::string name;
  ShaderInfos shader_info;
};

class Shaders {
public:
  Shaders()
  {
    auto simple_vertex_shader_resource = LOAD_RESOURCE(shader_simple_vertex_shader);
    auto simple_fragment_shader_resource = LOAD_RESOURCE(shader_simple_fragment_shader);
    auto rectangles_vertex_shader_resource = LOAD_RESOURCE(shader_rectangles_vertex_shader);
    auto rectangles_fragment_shader_resource = LOAD_RESOURCE(shader_rectangles_fragment_shader);
    auto font_vertex_shader_resource = LOAD_RESOURCE(shader_font_vertex_shader);
    auto font_fragment_shader_resource = LOAD_RESOURCE(shader_font_fragment_shader);
    // auto phong_vertex_shader_resource = LOAD_RESOURCE(shader_phong_vertex_shader);
    // auto phong_fragment_shader_resource = LOAD_RESOURCE(shader_phong_fragment_shader);

    shaders.emplace_back(
        Shader::ShaderSources{simple_vertex_shader_resource.toString(),
                              simple_fragment_shader_resource.toString()},
        "Simple Shader");
    shaders.emplace_back(
        Shader::ShaderSources{rectangles_vertex_shader_resource.toString(),
                              rectangles_fragment_shader_resource.toString()},
        "Rectangles Shader");
    shaders.emplace_back(
        Shader::ShaderSources{font_vertex_shader_resource.toString(),
                              font_fragment_shader_resource.toString()},
        "Font Shader");
    // shaders.push_back(Shader(phong_vertex_shader_resource.toString(),
    // phong_fragment_shader_resource.toString(), std::string("Phong Shader")));

    PrintInfo();
  }

  void PrintInfo() const
  {
    for (const auto &shader : shaders) {
      shader.PrintShaderInfo();
    }
  }

  Shader *GetShader(size_t index)
  {
    if (index < shaders.size()) {
      return &shaders[index];
    }
    throw std::invalid_argument("Shader index out of bounds");
  }

  std::optional<Shader *> search_shader(const std::string &search_name)
  {
    for (auto &shader : shaders) {
      if (shader.get_name() == search_name) {
        return std::optional<Shader *>{&shader};
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] size_t GetNumberShaders() const { return shaders.size(); }

private:
  std::vector<Shader> shaders;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_SHADERS_HPP
