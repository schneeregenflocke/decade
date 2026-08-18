#ifndef SHADERS_INFO_HPP
#define SHADERS_INFO_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <cstdint>
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
             InfoParams params);

  void UpdateTypeInfo();

  [[nodiscard]] InfoType GetInfoType() const;
  [[nodiscard]] const std::string& GetName() const;
  [[nodiscard]] GLint GetLocation() const;
  [[nodiscard]] GLint GetSize() const;
  [[nodiscard]] GLenum GetType() const;
  [[nodiscard]] size_t GetNumber() const;
  [[nodiscard]] size_t GetTypeSize() const;
  [[nodiscard]] const std::string& GetTypeString() const;

  // Whether a vertex array can be fed this type as floats — the vector types.
  // A uniform-only type (a matrix, a sampler) answers false, and so does one
  // the table below does not know: without that, an unknown type would reach
  // glVertexAttribFormat as zero components of zero bytes.
  [[nodiscard]] bool IsFloatVector() const;

  void Print() const;

 private:
  InfoType info_type_{InfoType::None};
  std::string name_;
  GLint location_{0};
  GLint size_{0};
  GLenum type_{0};
  size_t number_{0};
  size_t type_size_{0};
  bool float_vector_{false};
  std::string type_str_;
};

class ShaderInfos {
 public:
  ShaderInfos() = default;

  void SetProgram(GLuint new_program);

  [[nodiscard]] const std::vector<ShaderInfo>& GetAttributesInfos() const;

  [[nodiscard]] int GetNumberAttributes() const;

  [[nodiscard]] int GetNumberUniforms() const;

  void PrintAttributesInfo() const;

  void PrintUniformsInfo() const;

 private:
  void SortAttributesInfo();

  void SortUniformsInfo();

  void GatherAttributesInfo();

  void GatherUniformsInfo();

  GLuint program_{0};
  std::vector<ShaderInfo> attribute_infos_;
  std::vector<ShaderInfo> uniform_infos_;
};
#endif  // SHADERS_INFO_HPP
