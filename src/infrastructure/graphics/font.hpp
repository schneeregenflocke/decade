#ifndef FONT_HPP
#define FONT_HPP

#include <epoxy/gl.h>

#include <cstddef>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "drawable.hpp"
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

  // A click counts to the nearer character edge: up to half the advance width
  // it still belongs before the character.
  static constexpr float kHalfAdvance = 0.5F;

  explicit Font(const std::string& filepath);

  ~Font();

  Font(const Font&) = delete;
  Font& operator=(const Font&) = delete;
  Font(Font&&) = delete;
  Font& operator=(Font&&) = delete;

  // Memoised glyph lookup. Logically const (a cache), so the cache is mutable;
  // references into it stay valid across later insertions because the cache is
  // node-based (std::unordered_map).
  [[nodiscard]] const Letter& GetLetter(char32_t code_point) const;

  [[nodiscard]] float TextWidth(const std::string& text, float size) const;

  // The width of the first `count` code points — the measure the text editor
  // computes in: the cursor sits behind character `count`.
  [[nodiscard]] float TextWidth(const std::u32string& text, float size,
                                std::size_t count) const;

  // The inverse: the cursor index a click `offset` right of the text start
  // means. The nearer of the two character edges wins, so a click on the left
  // half of a character lands before it.
  [[nodiscard]] std::size_t IndexAtOffset(const std::u32string& text,
                                          float size, float offset) const;

  [[nodiscard]] float TextHeight(float size) const;

  // The largest em size at which `text` fits into `cell`: from the cell height
  // first, then taken back to the cell width where needed.
  [[nodiscard]] float AdjustTextSize(const RectF& cell, const std::string& text,
                                     TextScale scale) const;

 private:
  void InitFreetype();

  // Releases the FreeType resources without throwing — it runs from the
  // destructor, so failures are logged rather than propagated.
  void ReleaseFreetype() noexcept;

  void PrintVersion();

  void LoadFont(const std::string& file_path);

  void ConfigureFace();

  // Rasterises a single code point and uploads it as a GL_RED texture. Const
  // because it only feeds the memoising cache; it mutates the shared FreeType
  // glyph slot and GL state, not the logical state of the font.
  [[nodiscard]] Letter RenderGlyph(char32_t code_point) const;

  static constexpr float kFreeTypeFixedScale = 64.0F;
  static constexpr FT_UInt kFontPixelHeight = 2048;

  FT_Library ft_library_{nullptr};
  FT_Face ft_face_{nullptr};
  FT_UInt font_pixel_height_{kFontPixelHeight};
  mutable std::unordered_map<char32_t, Letter> glyph_cache_;
};

class FontShape : public Shape {
 public:
  explicit FontShape(Shader& shader_in);

  [[nodiscard]] DrawableKind Kind() const override;

  void SetFont(std::shared_ptr<Font> font_ptr);

  void SetColor(const glm::vec4& new_color);

  // The drawn text and its em size in page millimetres — the same values the
  // geometry came out of. The scene snapshot shows them without having to
  // compute back from the vertex buffers.
  [[nodiscard]] const std::string& Text() const;

  [[nodiscard]] float FontSize() const;

  void SetShape(const std::string& text, const glm::vec3& position, float size);

  void SetShapeCentered(const std::string& text, const glm::vec3& position,
                        float size);

  void Draw(const glm::mat4& model) const override;

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
  std::string text_;
  float font_size_{kZero};
};
#endif  // FONT_HPP
