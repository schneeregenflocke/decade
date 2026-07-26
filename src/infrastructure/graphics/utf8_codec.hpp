#ifndef UTF8_CODEC_HPP
#define UTF8_CODEC_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "unicode.hpp"

// Infrastructure: die Umrechnung zwischen UTF-8 und Codepoints, an einem Ort.
// Zwei Seiten brauchen sie: der Textrenderer, der je Codepoint eine Glyphe
// holt, und der Texteditor, der in Codepoints rechnet (ein Umlaut = ein
// Schritt). ICU trägt die Unicode-Arbeit.

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

// Gegenstück zu DecodeUtf8 für den Rückweg: Der Texteditor rechnet in
// Codepoints, gespeichert und gezeichnet wird UTF-8.
[[nodiscard]] inline std::string EncodeUtf8(const std::u32string& code_points) {
  icu::UnicodeString utf16;
  for (const char32_t code_point : code_points) {
    utf16.append(static_cast<UChar32>(code_point));
  }
  std::string text;
  utf16.toUTF8String(text);
  return text;
}

#endif  // UTF8_CODEC_HPP
