#ifndef FONT_HPP
#define FONT_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "freetype.hpp"
#include "icu.hpp"
#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"
#include "texture_object.hpp"

// Append the code points of the half-open UTF-16 range [begin, end) of an
// ICU string to out. char32At returns the full code point at a unit index; a
// code point above the BMP occupies two UTF-16 units, hence the variable step.
inline void AppendCodePoints(const icu::UnicodeString& text, std::int32_t begin,
                             std::int32_t end, std::vector<char32_t>& out) {
  constexpr UChar32 kBmpMax = 0xFFFF;
  for (std::int32_t index = begin; index < end;) {
    const UChar32 code_point = text.char32At(index);
    index += (code_point > kBmpMax) ? 2 : 1;
    out.push_back(static_cast<char32_t>(code_point));
  }
}

// Decode UTF-8 into a sequence of Unicode code points ready for glyph lookup.
//
// Strings reaching the renderer are UTF-8 (e.g. strftime month names like
// "März", or user-entered title text). ICU does the heavy lifting:
//   1. fromUTF8 turns the bytes into a UTF-16 string, replacing malformed
//      sequences with U+FFFD instead of producing stray glyphs.
//   2. NFC normalisation folds a base letter plus a combining mark (a + ◌̈)
//      into its precomposed form (ä) where one exists, so it renders as a
//      single glyph — FreeType has no shaping engine of its own.
//   3. A grapheme BreakIterator walks user-perceived characters; the code
//      points of each cluster are emitted in order. Code points that have no
//      precomposed form (and full complex-script shaping, e.g. Arabic/Indic)
//      would need HarfBuzz and are out of scope; they fall back to per-code
//      point glyphs, and a code point absent from the face renders as the
//      font's .notdef glyph.
[[nodiscard]] inline std::vector<char32_t> DecodeUtf8(const std::string& text) {
  std::vector<char32_t> code_points;
  if (text.empty()) {
    return code_points;
  }

  const icu::UnicodeString utf16 = icu::UnicodeString::fromUTF8(
      icu::StringPiece(text.data(), static_cast<std::int32_t>(text.size())));

  UErrorCode status = U_ZERO_ERROR;
  const icu::Normalizer2* nfc = icu::Normalizer2::getNFCInstance(status);
  // ICU's U_SUCCESS / U_FAILURE expand to UBool (signed char); compare against
  // 0 to get a real bool rather than relying on an implicit conversion.
  icu::UnicodeString normalized =
      (U_SUCCESS(status) != 0) ? nfc->normalize(utf16, status) : utf16;
  if (U_FAILURE(status) != 0) {
    normalized = utf16;  // fall back to the unnormalised text
  }

  code_points.reserve(static_cast<std::size_t>(normalized.countChar32()));

  status = U_ZERO_ERROR;
  const std::unique_ptr<icu::BreakIterator> grapheme_breaks(
      icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(),
                                                  status));
  if (U_FAILURE(status) != 0 || !grapheme_breaks) {
    AppendCodePoints(normalized, 0, normalized.length(), code_points);
    return code_points;
  }

  grapheme_breaks->setText(normalized);
  std::int32_t start = grapheme_breaks->first();
  for (std::int32_t end = grapheme_breaks->next();
       end != icu::BreakIterator::DONE;
       start = end, end = grapheme_breaks->next()) {
    AppendCodePoints(normalized, start, end, code_points);
  }

  return code_points;
}

struct Letter {
  Texture texture_object;
  glm::vec2 size{0.0F, 0.0F};
  glm::vec2 bearing{0.0F, 0.0F};
  float advance{0.0F};
};

// Renders glyphs from a font file on demand and caches them as GL textures.
//
// FreeType is kept alive for the whole lifetime of the object (RAII), because
// glyphs are rasterised lazily the first time their code point is requested —
// there is no fixed alphabet. A packed atlas is deliberately avoided: at
// kFontPixelHeight a single glyph is several megabytes, so an atlas would hold
// only a handful; one texture per used code point is both simpler and uses
// less memory for the typical (Latin) text this application renders.
class Font {
 public:
  struct TextScale {
    float height_ratio;
    float width_ratio;
  };

  explicit Font(const std::string& filepath) {
    InitFreetype();
    LoadFont(filepath);
    ConfigureFace();
  }

  ~Font() { ReleaseFreetype(); }

  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
  Font(Font&&) = delete;
  Font& operator=(Font&&) = delete;

  // Memoised glyph lookup. Logically const (a cache), so the cache is mutable;
  // references into it stay valid across later insertions because the cache is
  // node-based (std::unordered_map).
  [[nodiscard]] const Letter& GetLetter(char32_t code_point) const {
    const auto found = glyph_cache_.find(code_point);
    if (found != glyph_cache_.end()) {
      return found->second;
    }
    return glyph_cache_.emplace(code_point, RenderGlyph(code_point))
        .first->second;
  }

  [[nodiscard]] float TextWidth(const std::string& text, float size) const {
    float width = 0.0F;
    for (const char32_t code_point : DecodeUtf8(text)) {
      width += GetLetter(code_point).advance * size;
    }

    return width;
  }

  [[nodiscard]] float TextHeight(float size) const {
    constexpr std::array<std::array<char32_t, 2>, 3> kCharIntervals = {
        {{'0', '9'}, {'A', 'Z'}, {'a', 'z'}}};

    float height = 0.0F;
    for (const auto& char_interval : kCharIntervals) {
      for (char32_t code_point = char_interval[0];
           code_point <= char_interval[1]; ++code_point) {
        const float current_character_bearing =
            GetLetter(code_point).bearing[1] * size;
        height = std::max(height, current_character_bearing);
      }
    }

    return height;
  }

  [[nodiscard]] float AdjustTextSize(const rectf& cell, const std::string& text,
                                     TextScale scale) const {
    float font_size = cell.height() * scale.height_ratio;
    const auto text_width = TextWidth(text, font_size);
    const auto ratio = font_size / text_width;

    if (text_width > cell.width() * scale.width_ratio) {
      font_size = ratio * cell.width() * scale.width_ratio;
    }

    return font_size;
  }

 private:
  void InitFreetype() {
    const FT_Error ft_error = FT_Init_FreeType(&ft_library_);
    if (ft_error == FT_Err_Ok) {
      PrintVersion();
    } else {
      throw std::runtime_error(
          std::string("Freetype FT_Init_FreeType failed ") +
          std::to_string(ft_error));
    }
  }

  // Releases the FreeType resources without throwing — it runs from the
  // destructor, so failures are logged rather than propagated.
  void ReleaseFreetype() noexcept {
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

  void PrintVersion() {
    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Library_Version(ft_library_, &major, &minor, &patch);
    std::cout << "FreeType Version " << major << "." << minor << "." << patch
              << '\n';
  }

  void LoadFont(const std::string& file_path) {
    const FT_Error ft_error =
        FT_New_Face(ft_library_, file_path.c_str(), 0, &ft_face_);
    if (ft_error == FT_Err_Ok) {
      std::cout << "ft_face->family_name " << ft_face_->family_name << '\n';
    } else {
      throw std::runtime_error(std::string("Freetype FT_New_Face failed ") +
                               std::to_string(ft_error));
    }
  }

  void ConfigureFace() {
    const FT_Error ft_error =
        FT_Set_Pixel_Sizes(ft_face_, 0, font_pixel_height_);
    if (ft_error != FT_Err_Ok) {
      throw std::runtime_error(
          std::string("Freetype FT_Set_Pixel_Sizes failed ") +
          std::to_string(ft_error));
    }
  }

  // Rasterises a single code point and uploads it as a GL_RED texture. Const
  // because it only feeds the memoising cache; it mutates the shared FreeType
  // glyph slot and GL state, not the logical state of the font.
  [[nodiscard]] Letter RenderGlyph(char32_t code_point) const {
    const FT_Error load_char_error = FT_Load_Char(
        ft_face_, static_cast<FT_ULong>(code_point), FT_LOAD_RENDER);
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
    letter.size =
        glm::vec2(static_cast<float>(bitmap.width) / float_font_height,
                  static_cast<float>(bitmap.rows) / float_font_height);
    letter.bearing =
        glm::vec2(static_cast<float>(glyph->bitmap_left) / float_font_height,
                  static_cast<float>(glyph->bitmap_top) / float_font_height);
    letter.advance = static_cast<float>(glyph->advance.x) /
                     kFreeTypeFixedScale / float_font_height;

    return letter;
  }

  static constexpr float kFreeTypeFixedScale = 64.0F;
  static constexpr FT_UInt kFontPixelHeight = 2048;

  FT_Library ft_library_{nullptr};
  FT_Face ft_face_{nullptr};
  FT_UInt font_pixel_height_{kFontPixelHeight};
  mutable std::unordered_map<char32_t, Letter> glyph_cache_;
};

class FontShape : public Shape {
 public:
  explicit FontShape(Shader* shader_ptr_in) : Shape(shader_ptr_in) {}

  void set_font(std::shared_ptr<Font> font_ptr) { font_ = std::move(font_ptr); }

  void set_color(const glm::vec4& new_color) { color_ = new_color; }

  void set_shape(const std::string& text, const glm::vec3& position,
                 float size) {
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

    set_buffer(BufferIndex{0}, static_cast<GLsizei>(positions_.size()),
               positions_.data());
    set_buffer(BufferIndex{1}, static_cast<GLsizei>(texture_positions_.size()),
               texture_positions_.data());
  }

  void set_shape_centered(const std::string& text, const glm::vec3& position,
                          float size) {
    const auto half_width = font_->TextWidth(text, size) * kHalf;
    const auto half_height = font_->TextHeight(size) * kHalf;

    set_shape(text, position - glm::vec3(half_width, half_height, kZero), size);
  }

  void draw() const override {
    shader()->UseProgram();

    shader()->SetUniform("texture_color", color_);

    vao_ref().bind();

    glActiveTexture(GL_TEXTURE0);

    for (size_t index = 0; index < text_textures_.size(); ++index) {
      glBindTexture(GL_TEXTURE_2D, text_textures_[index]);
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

  std::vector<glm::vec3> positions_;
  std::vector<glm::vec2> texture_positions_;
  std::vector<GLuint> text_textures_;
  std::shared_ptr<Font> font_;
  glm::vec4 color_{kZero, kZero, kZero, kOne};
};
#endif  // FONT_HPP
