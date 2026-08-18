#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <functional>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "mvp_matrices.hpp"
#include "shaders_info.hpp"

class Shader {
 public:
  struct ShaderSources {
    std::string vertex;
    std::string fragment;
  };

  explicit Shader(const ShaderSources& sources, std::string name_in);

  [[nodiscard]] GLuint GetProgram() const;

  [[nodiscard]] std::string GetName() const;

  void UseProgram() const;

  void SetUniform(const std::string& uniform_name,
                  const glm::mat4& matrix) const;

  void SetUniform(const std::string& uniform_name,
                  const glm::vec4& vector) const;

  void PrintShaderInfo() const;

  [[nodiscard]] const ShaderInfos& GetShaderInfo() const;

  [[nodiscard]] const std::vector<ShaderInfo>& GetShaderAttributesInfos() const;

 private:
  struct ShaderHandles {
    GLuint vertex;
    GLuint fragment;
  };

  // Uniform locations are stable after linking; the cache spares the
  // glGetUniformLocation string lookup that would otherwise fall due per shape
  // and frame.
  GLint UniformLocation(const std::string& uniform_name) const;

  void CompileProgram(const ShaderSources& sources);

  void LinkShaders(const ShaderHandles& handles);

  static GLuint CompileShader(const std::string& source, GLenum shader_type,
                              const std::string& label);

  GLuint program_{0};
  std::string name_;
  ShaderInfos shader_info_;
  mutable std::unordered_map<std::string, GLint> uniform_locations_;
};

class Shaders {
 public:
  Shaders();

  // Every shader with its attributes and uniforms — diagnosis, so it hangs on
  // --debug-log. One guard for the whole chain: PrintShaderInfo and the
  // ShaderInfo printers below it have no other caller.
  void PrintInfo() const;

  // Projection and view are frame-global, so they are set for all shaders at
  // once. The iteration lives here, with the shaders, rather than at the one
  // caller that would otherwise need an index into them.
  void SetCameraUniforms(const MVP& mvp);

  // A reference_wrapper and not a pointer: the optional already carries the
  // absence, so a pointer inside it would be nullable twice over.
  std::optional<std::reference_wrapper<Shader>> SearchShader(
      const std::string& search_name);

 private:
  std::vector<Shader> shaders_;
};
#endif  // SHADERS_HPP
