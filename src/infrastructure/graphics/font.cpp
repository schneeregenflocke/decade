#include "font.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../common/debug_log.hpp"
#include "drawable.hpp"
#include "freetype.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"
#include "utf8_codec.hpp"
#include "vertex_objects.hpp"

Font::Font(const std::string& filepath) {
  InitFreetype();
  LoadFont(filepath);
  ConfigureFace();
}

Font::~Font() { ReleaseFreetype(); }

const Letter& Font::GetLetter(char32_t code_point) const {
  const auto found = glyph_cache_.find(code_point);
  if (found != glyph_cache_.end()) {
    return found->second;
  }
  return glyph_cache_.emplace(code_point, RenderGlyph(code_point))
      .first->second;
}

float Font::TextWidth(const std::string& text, float size) const {
  float width = 0.0F;
  for (const char32_t code_point : DecodeUtf8(text)) {
    width += GetLetter(code_point).advance * size;
  }

  return width;
}

float Font::TextWidth(const std::u32string& text, float size,
                      std::size_t count) const {
  float width = 0.0F;
  for (std::size_t index = 0; index < std::min(count, text.size()); ++index) {
    width += GetLetter(text[index]).advance * size;
  }

  return width;
}

std::size_t Font::IndexAtOffset(const std::u32string& text, float size,
                                float offset) const {
  float left = 0.0F;
  for (std::size_t index = 0; index < text.size(); ++index) {
    const float advance = GetLetter(text[index]).advance * size;
    if (offset < left + (advance * kHalfAdvance)) {
      return index;
    }
    left += advance;
  }

  return text.size();
}

float Font::TextHeight(float size) const {
  constexpr std::array<std::array<char32_t, 2>, 3> kCharIntervals = {
      {{'0', '9'}, {'A', 'Z'}, {'a', 'z'}}};

  float height = 0.0F;
  for (const auto& char_interval : kCharIntervals) {
    for (char32_t code_point = char_interval[0]; code_point <= char_interval[1];
         ++code_point) {
      const float current_character_bearing =
          GetLetter(code_point).bearing[1] * size;
      height = std::max(height, current_character_bearing);
    }
  }

  return height;
}

float Font::AdjustTextSize(const RectF& cell, const std::string& text,
                           TextScale scale) const {
  const float font_size = cell.Height() * scale.height_ratio;
  const float text_width = TextWidth(text, font_size);
  const float available_width = cell.Width() * scale.width_ratio;

  if (text_width <= available_width) {
    return font_size;
  }
  return font_size / text_width * available_width;
}

void Font::InitFreetype() {
  const FT_Error ft_error = FT_Init_FreeType(&ft_library_);
  if (ft_error == FT_Err_Ok) {
    PrintVersion();
  } else {
    throw std::runtime_error(std::string("Freetype FT_Init_FreeType failed ") +
                             std::to_string(ft_error));
  }
}

void Font::ReleaseFreetype() noexcept {
  if (ft_face_ != nullptr) {
    const FT_Error ft_error = FT_Done_Face(ft_face_);
    if (ft_error != FT_Err_Ok) {
      std::cerr << "Freetype FT_Done_Face failed " << ft_error << '\n';
    }
    ft_face_ = nullptr;
  }
  if (ft_library_ != nullptr) {
    const FT_Error ft_error = FT_Done_FreeType(ft_library_);
    if (ft_error != FT_Err_Ok) {
      std::cerr << "Freetype FT_Done_FreeType failed " << ft_error << '\n';
    }
    ft_library_ = nullptr;
  }
}

void Font::PrintVersion() {
  FT_Int major = 0;
  FT_Int minor = 0;
  FT_Int patch = 0;
  FT_Library_Version(ft_library_, &major, &minor, &patch);
  if (decade_debug::LogEnabled()) {
    std::cout << "FreeType Version " << major << "." << minor << "." << patch
              << '\n';
  }
}

void Font::LoadFont(const std::string& file_path) {
  const FT_Error ft_error =
      FT_New_Face(ft_library_, file_path.c_str(), 0, &ft_face_);
  if (ft_error == FT_Err_Ok) {
    if (decade_debug::LogEnabled()) {
      std::cout << "ft_face->family_name " << ft_face_->family_name << '\n';
    }
  } else {
    throw std::runtime_error(std::string("Freetype FT_New_Face failed ") +
                             std::to_string(ft_error));
  }
}

void Font::ConfigureFace() {
  const FT_Error ft_error = FT_Set_Pixel_Sizes(ft_face_, 0, font_pixel_height_);
  if (ft_error != FT_Err_Ok) {
    throw std::runtime_error(
        std::string("Freetype FT_Set_Pixel_Sizes failed ") +
        std::to_string(ft_error));
  }
}

Letter Font::RenderGlyph(char32_t code_point) const {
  const FT_Error load_char_error =
      FT_Load_Char(ft_face_, static_cast<FT_ULong>(code_point), FT_LOAD_RENDER);
  if (load_char_error != FT_Err_Ok) {
    throw std::runtime_error(std::string("Freetype FT_Load_Char failed ") +
                             std::to_string(load_char_error));
  }
  if (ft_face_->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
    throw std::runtime_error(
        std::string("Freetype glyph->format != FT_GLYPH_FORMAT_BITMAP"));
  }

  const FT_GlyphSlotRec_* const glyph = ft_face_->glyph;
  const auto& bitmap = glyph->bitmap;

  Letter letter;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, letter.texture_object.Name());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, static_cast<GLsizei>(bitmap.width),
               static_cast<GLsizei>(bitmap.rows), 0, GL_RED, GL_UNSIGNED_BYTE,
               bitmap.buffer);
  glBindTexture(GL_TEXTURE_2D, 0);

  const auto float_font_height = static_cast<float>(font_pixel_height_);
  letter.size = glm::vec2(static_cast<float>(bitmap.width) / float_font_height,
                          static_cast<float>(bitmap.rows) / float_font_height);
  letter.bearing =
      glm::vec2(static_cast<float>(glyph->bitmap_left) / float_font_height,
                static_cast<float>(glyph->bitmap_top) / float_font_height);
  letter.advance = static_cast<float>(glyph->advance.x) / kFreeTypeFixedScale /
                   float_font_height;

  return letter;
}

FontShape::FontShape(Shader& shader_in) : Shape(shader_in) {}

DrawableKind FontShape::Kind() const { return DrawableKind::kText; }

void FontShape::SetFont(std::shared_ptr<Font> font_ptr) {
  font_ = std::move(font_ptr);
}

void FontShape::SetColor(const glm::vec4& new_color) { color_ = new_color; }

const std::string& FontShape::Text() const { return text_; }

float FontShape::FontSize() const { return font_size_; }

void FontShape::SetShape(const std::string& text, const glm::vec3& position,
                         float size) {
  text_ = text;
  font_size_ = size;
  const auto glyphs = DecodeUtf8(text);
  const auto glyph_count = glyphs.size();
  positions_.resize(glyph_count * kVerticesPerGlyph);
  texture_positions_.resize(glyph_count * kVerticesPerGlyph);

  text_textures_.resize(glyph_count);

  float current_x = position[0];
  const float current_y = position[1];

  for (size_t index = 0; index < glyph_count; ++index) {
    const Letter& letter = font_->GetLetter(glyphs[index]);

    const GLuint texture = letter.texture_object.Name();
    text_textures_[index] = texture;

    const GLfloat xpos = current_x + (letter.bearing[0] * size);
    const GLfloat ypos =
        current_y - ((letter.size[1] - letter.bearing[1]) * size);

    const GLfloat width = letter.size[0] * size;
    const GLfloat height = letter.size[1] * size;

    const auto base = index * kVerticesPerGlyph;
    positions_[base + 0] = glm::vec3(xpos, ypos + height, kZero);
    positions_[base + 1] = glm::vec3(xpos, ypos, kZero);
    positions_[base + 2] = glm::vec3(xpos + width, ypos, kZero);
    positions_[base + 3] = glm::vec3(xpos, ypos + height, kZero);
    positions_[base + 4] = glm::vec3(xpos + width, ypos, kZero);
    const auto last = base + (kVerticesPerGlyph - 1);
    positions_[last] = glm::vec3(xpos + width, ypos + height, kZero);

    texture_positions_[base + 0] = glm::vec2(kZero, kZero);
    texture_positions_[base + 1] = glm::vec2(kZero, kOne);
    texture_positions_[base + 2] = glm::vec2(kOne, kOne);
    texture_positions_[base + 3] = glm::vec2(kZero, kZero);
    texture_positions_[base + 4] = glm::vec2(kOne, kOne);
    texture_positions_[last] = glm::vec2(kOne, kZero);

    current_x += letter.advance * size;
  }

  SetBuffer(BufferIndex{0}, std::span<const glm::vec3>(positions_));
  SetBuffer(BufferIndex{1}, std::span<const glm::vec2>(texture_positions_));

  if (positions_.empty()) {
    SetLocalBounds({});
  } else {
    float left = positions_[0].x;
    float right = positions_[0].x;
    float bottom = positions_[0].y;
    float top = positions_[0].y;
    for (const auto& vertex : positions_) {
      left = std::min(left, vertex.x);
      right = std::max(right, vertex.x);
      bottom = std::min(bottom, vertex.y);
      top = std::max(top, vertex.y);
    }
    SetLocalBounds(RectF(left, right, bottom, top));
  }
}

void FontShape::SetShapeCentered(const std::string& text,
                                 const glm::vec3& position, float size) {
  const auto half_width = font_->TextWidth(text, size) * kHalf;
  const auto half_height = font_->TextHeight(size) * kHalf;

  SetShape(text, position - glm::vec3(half_width, half_height, kZero), size);
}

void FontShape::Draw(const glm::mat4& model) const {
  GetShader().UseProgram();
  GetShader().SetUniform("model", model);

  GetShader().SetUniform("texture_color", color_);

  VaoRef().Bind();

  glActiveTexture(GL_TEXTURE0);

  for (size_t index = 0; index < text_textures_.size(); ++index) {
    glBindTexture(GL_TEXTURE_2D, text_textures_[index]);
    glDrawArrays(GL_TRIANGLES, static_cast<GLint>(index * kVerticesPerGlyph),
                 static_cast<GLsizei>(kVerticesPerGlyph));
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  VertexArrayObject::Unbind();
}
