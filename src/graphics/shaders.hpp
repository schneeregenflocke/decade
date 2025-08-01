/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// #include "Resource.h"
#include "shaders_info.hpp"
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Shader {
public:
  Shader(const std::string &vertex_shader_source, const std::string &fragment_shader_source,
         std::string name)
      : name(std::move(name))
  {
    CompileProgram(vertex_shader_source, fragment_shader_source);

    shader_info.SetProgram(program);
  }

  [[nodiscard]] GLuint GetProgram() const { return program; }

  [[nodiscard]] std::string get_name() const { return name; }

  void UseProgram() const { glUseProgram(program); }

  void SetUniform(const std::string &name, const glm::mat4 &matrix) const
  {
    auto location = glGetUniformLocation(program, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
  }

  void SetUniform(const std::string &name, const glm::vec4 &vector) const
  {
    auto location = glGetUniformLocation(program, name.c_str());
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
  void CompileProgram(const std::string &vertex_shader_source,
                      const std::string &fragment_shader_source)
  {
    GLuint vertex_shader = CompileShader(vertex_shader_source, GL_VERTEX_SHADER);
    GLuint fragment_shader = CompileShader(fragment_shader_source, GL_FRAGMENT_SHADER);

    LinkShaders(vertex_shader, fragment_shader);
  }

  void LinkShaders(const GLuint vertex_shader, const GLuint fragment_shader)
  {
    program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    glValidateProgram(program);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    std::cout << "GL_LINK_STATUS: " << program << " " << std::boolalpha << static_cast<bool>(status)
              << std::noboolalpha << '\n';

    GLint info_lenght = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_lenght);
    if (info_lenght > 0) {
      char *info_log = new char[info_lenght];
      glGetProgramInfoLog(program, info_lenght, nullptr, info_log);
      std::cout << info_log;
      delete[] info_log;
    }

    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }

  GLuint CompileShader(const std::string &source, const GLuint shadertype)
  {
    GLuint shader = glCreateShader(shadertype);
    const char *source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    std::cout << "GL_COMPILE_STATUS: " << shader << " " << std::boolalpha
              << static_cast<bool>(status) << std::noboolalpha << '\n';

    GLint info_lenght = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_lenght);
    if (info_lenght > 0) {
      char *info_log = new char[info_lenght];
      glGetShaderInfoLog(shader, info_lenght, nullptr, info_log);
      std::cout << info_log;
      delete[] info_log;
    }

    return shader;
  }

  GLuint program{};
  std::string name;
  ShaderInfos shader_info;
};

class Shaders {
public:
  Shaders()
  {
    std::string basic_vs("../shaders/basic_vertex_shader.glsl");
    std::string basic_fs("../shaders/basic_fragment_shader.glsl");
    std::string font_vs("../shaders/font_vertex_shader.glsl");
    std::string font_fs("../shaders/font_fragment_shader.glsl");
    std::string rect_vs("../shaders/rectangles_vertex_shader.glsl");
    std::string rect_fs("../shaders/rectangles_fragment_shader.glsl");

    shaders.emplace_back(read_file(basic_vs), read_file(basic_fs), "Basic Shader");
    shaders.emplace_back(read_file(font_vs), read_file(font_fs), "Font Shader");
    shaders.emplace_back(read_file(rect_vs), read_file(rect_fs), "Rectangles Shader");

    PrintInfo();
  }

  static std::string read_file(const std::filesystem::path &path)
  {
    if (!std::filesystem::exists(path)) {
      throw std::runtime_error("Shader file does not exist: " + path.string());
    }
    std::ifstream file(path, std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(file), {}};
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

  Shader *search_shader(const std::string &search_name)
  {

    for (auto &shader : shaders) {
      if (shader.get_name() == search_name) {
        return &shader;
      }
    }
    throw std::invalid_argument("Shader not found: " + search_name);
    return nullptr;
  }

  [[nodiscard]] size_t GetNumberShaders() const { return shaders.size(); }

private:
  std::vector<Shader> shaders;
};
