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

#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_FONT_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_FONT_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <epoxy/gl.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "freetype.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"
#include "texture_object.hpp"
struct Letter {
  Texture texture_object;
  glm::vec2 size{0.0F, 0.0F};
  glm::vec2 bearing{0.0F, 0.0F};
  float advance{0.0F};
};

class Font {
public:
  /*Font(const std::vector<unsigned char>& font_data) :
          ft_library(nullptr), ft_face(nullptr), letters(256)
  {
          InitFreetype();
          LoadFont(font_data);
          LoadTextures();
          ReleaseFreetype();
  }*/

  struct TextScale {
    float height_ratio;
    float width_ratio;
  };

  explicit Font(const std::string &filepath) : letters(kDefaultGlyphCount)
  {
    InitFreetype();
    LoadFont(filepath);
    LoadTextures();
    ReleaseFreetype();
  }

  [[nodiscard]] const Letter &GetLetterRef(const unsigned char index) const
  {
    return letters.at(index);
  }

  [[nodiscard]] float TextWidth(const std::string &text, float size) const
  {
    float width = 0.0F;
    for (const unsigned char letter_number : text) {
      width += GetLetterRef(letter_number).advance * size;
    }

    return width;
  }

  [[nodiscard]] float TextHeight(float size) const
  {
    constexpr std::array<std::array<unsigned char, 2>, 3> kCharIntervals = {
        {{'0', '9'}, {'A', 'Z'}, {'a', 'z'}}};
    std::vector<unsigned char> index_list;

    for (const auto &char_interval : kCharIntervals) {
      for (unsigned char index = char_interval[0]; index <= char_interval[1]; ++index) {
        index_list.push_back(index);
      }
    }

    float height = 0.0F;
    for (const auto index : index_list) {
      const float current_character_bearing = GetLetterRef(index).bearing[1] * size;
      height = std::max(height, current_character_bearing);
    }

    return height;
  }

  [[nodiscard]] float AdjustTextSize(const rectf &cell, const std::string &text,
                                     TextScale scale) const
  {
    float font_size = cell.height() * scale.height_ratio;
    const auto text_width = TextWidth(text, font_size);
    const auto ratio = font_size / text_width;

    if (text_width > cell.width() * scale.width_ratio) {
      font_size = ratio * cell.width() * scale.width_ratio;
    }

    return font_size;
  }

private:
  void InitFreetype()
  {
    const FT_Error ft_error = FT_Init_FreeType(&ft_library);
    if (ft_error == FT_Err_Ok) {
      PrintVersion();
    } else {
      throw std::runtime_error(std::string("Freetype FT_Init_FreeType failed ") +
                               std::to_string(ft_error));
    }
  }

  void ReleaseFreetype()
  {
    FT_Error ft_error = FT_Done_Face(ft_face);
    if (ft_error != FT_Err_Ok) {
      throw std::runtime_error(std::string("Freetype FT_Done_Face failed ") +
                               std::to_string(ft_error));
    }
    ft_face = nullptr;
    ft_error = FT_Done_FreeType(ft_library);
    if (ft_error != FT_Err_Ok) {
      throw std::runtime_error(std::string("Freetype FT_Done_FreeType failed ") +
                               std::to_string(ft_error));
    }
    ft_library = nullptr;
  }

  void PrintVersion()
  {
    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Library_Version(ft_library, &major, &minor, &patch);
    std::cout << "FreeType Version " << major << "." << minor << "." << patch << '\n';
  }

  void LoadFont(const std::string &file_path)
  {
    const FT_Error ft_error = FT_New_Face(ft_library, file_path.c_str(), 0, &ft_face);
    if (ft_error == FT_Err_Ok) {
      std::cout << "ft_face->family_name " << ft_face->family_name << '\n';
    } else {
      throw std::runtime_error(std::string("Freetype FT_New_Face failed ") +
                               std::to_string(ft_error));
    }
  }

  /*void LoadFont(const std::vector<unsigned char>& font_data)
  {
          FT_Error ft_error = FT_New_Memory_Face(ft_library, font_data.data(), font_data.size(), 0,
  &ft_face); if (ft_error == FT_Err_Ok)
          {
                  std::cout << "ft_face->family_name " << ft_face->family_name <<  '\n';
          }
          else
          {
                  throw std::runtime_error(std::string("Freetype FT_New_Memory_Face failed ") +
  std::to_string(ft_error));
          }
  }*/

  void LoadTextures()
  {
    const FT_UInt font_pixel_height = kFontPixelHeight;
    // const FT_UInt font_pixel_height = 128;
    const FT_Error ft_error = FT_Set_Pixel_Sizes(ft_face, 0, font_pixel_height);

    // FT_CONFIG_OPTION_ERROR_STRINGS, FT_DEBUG_LEVEL_ERROR
    const auto *ft_error_string = FT_Error_String(ft_error);
    const std::string error_string =
        (ft_error_string != nullptr) ? std::string(ft_error_string, std::strlen(ft_error_string))
                                      : std::string("Unknown error");
    if (ft_error != FT_Err_Ok) {
      std::cout << "FreeType Error: " << error_string << '\n';
      throw std::runtime_error(std::string("Freetype FT_Set_Pixel_Sizes failed ") +
                               std::to_string(ft_error));
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);

    // const size_t number_letters = 256;
    // letters.resize(number_letters);
    for (size_t index = 0; index < letters.size(); ++index) {
      const FT_Error ft_error = FT_Load_Char(ft_face, index, FT_LOAD_RENDER);
      if (ft_error != FT_Err_Ok) {
        throw std::runtime_error(std::string("Freetype FT_Load_Char failed ") +
                                 std::to_string(ft_error));
      }
      if (ft_face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        throw std::runtime_error(std::string("Freetype glyph->format != FT_GLYPH_FORMAT_BITMAP"));
      }

      const auto &bitmap = ft_face->glyph->bitmap;
      const auto bitmap_width = bitmap.width;
      const auto bitmap_height = bitmap.rows;

      const GLuint texture = letters[index].texture_object.Name();
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, static_cast<GLsizei>(bitmap_width),
                   static_cast<GLsizei>(bitmap_height), 0, GL_RED, GL_UNSIGNED_BYTE,
                   bitmap.buffer);
      // glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);

      const auto float_font_height = static_cast<float>(font_pixel_height);

      const float sizex = static_cast<float>(bitmap_width) / float_font_height;
      const float sizey = static_cast<float>(bitmap_height) / float_font_height;
      letters[index].size = glm::vec2(sizex, sizey);

      const float bearingx = static_cast<float>(ft_face->glyph->bitmap_left) / float_font_height;
      const float bearingy = static_cast<float>(ft_face->glyph->bitmap_top) / float_font_height;
      letters[index].bearing = glm::vec2(bearingx, bearingy);
      letters[index].advance =
          static_cast<float>(ft_face->glyph->advance.x) / kFreeTypeFixedScale / float_font_height;

    }
  }

  static constexpr size_t kDefaultGlyphCount = 256;
  static constexpr float kFreeTypeFixedScale = 64.0F;
  static constexpr FT_UInt kFontPixelHeight = 2048;

  FT_Library ft_library{nullptr};
  FT_Face ft_face{nullptr};
  std::vector<Letter> letters;
};

class FontShape : public Shape {
public:
  explicit FontShape(Shader *shader_ptr) : Shape(shader_ptr) {}

  void set_font(std::shared_ptr<Font> font_ptr) { font = std::move(font_ptr); }

  void set_shape(const std::string &text, const glm::vec3 &position, float size)
  {
    const auto glyph_count = text.size();
    positions.resize(glyph_count * kVerticesPerGlyph);
    texture_positions.resize(glyph_count * kVerticesPerGlyph);

    text_textures.resize(glyph_count);

    float current_x = position[0];
    const float current_y = position[1];

    for (size_t index = 0; index < glyph_count; ++index) {
      const auto letter_index = static_cast<unsigned char>(text[index]);

      const GLuint texture = font->GetLetterRef(letter_index).texture_object.Name();
      text_textures[index] = texture;

      const GLfloat xpos =
          current_x + (font->GetLetterRef(letter_index).bearing[0] * size);
      const GLfloat ypos =
          current_y -
          ((font->GetLetterRef(letter_index).size[1] -
            font->GetLetterRef(letter_index).bearing[1]) *
           size);

      const GLfloat width = font->GetLetterRef(letter_index).size[0] * size;
      const GLfloat height = font->GetLetterRef(letter_index).size[1] * size;

      const auto base = index * kVerticesPerGlyph;
      positions[base + 0] = glm::vec3(xpos, ypos + height, kZero);
      positions[base + 1] = glm::vec3(xpos, ypos, kZero);
      positions[base + 2] = glm::vec3(xpos + width, ypos, kZero);
      positions[base + 3] = glm::vec3(xpos, ypos + height, kZero);
      positions[base + 4] = glm::vec3(xpos + width, ypos, kZero);
      const auto last = base + (kVerticesPerGlyph - 1);
      positions[last] = glm::vec3(xpos + width, ypos + height, kZero);

      texture_positions[base + 0] = glm::vec2(kZero, kZero);
      texture_positions[base + 1] = glm::vec2(kZero, kOne);
      texture_positions[base + 2] = glm::vec2(kOne, kOne);
      texture_positions[base + 3] = glm::vec2(kZero, kZero);
      texture_positions[base + 4] = glm::vec2(kOne, kOne);
      texture_positions[last] = glm::vec2(kOne, kZero);

      current_x += font->GetLetterRef(letter_index).advance * size;
    }

    set_buffer(BufferIndex{0}, static_cast<GLsizei>(positions.size()), positions.data());
    set_buffer(BufferIndex{1}, static_cast<GLsizei>(texture_positions.size()),
               texture_positions.data());
  }

  void set_shape_centered(const std::string &text, const glm::vec3 &position, float size)
  {
    const auto half_width = font->TextWidth(text, size) * kHalf;
    const auto half_height = font->TextHeight(size) * kHalf;

    set_shape(text, position - glm::vec3(half_width, half_height, kZero), size);
  }

  void draw() const override
  {
    shader()->UseProgram();

    const glm::vec4 color(kZero, kZero, kZero, kOne);
    shader()->SetUniform("texture_color", color);

    vao_ref().bind();

    glActiveTexture(GL_TEXTURE0);

    for (size_t index = 0; index < text_textures.size(); ++index) {
      glBindTexture(GL_TEXTURE_2D, text_textures[index]);
      glDrawArrays(GL_TRIANGLES, static_cast<GLint>(index * kVerticesPerGlyph),
                   static_cast<GLsizei>(kVerticesPerGlyph));
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    VertexArrayObject::Unbind();
  }

private:
  static constexpr float kZero = 0.0F;
  static constexpr float kOne = 1.0F;
  static constexpr float kHalf = 0.5F;
  static constexpr size_t kVerticesPerGlyph = 6;

  std::vector<glm::vec3> positions;
  std::vector<glm::vec2> texture_positions;
  std::vector<GLuint> text_textures;
  std::shared_ptr<Font> font;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_FONT_HPP
