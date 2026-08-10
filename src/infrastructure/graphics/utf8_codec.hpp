#ifndef UTF8_CODEC_HPP
#define UTF8_CODEC_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "unicode.hpp"

// Infrastructure: the conversion between UTF-8 and code points, in one place.
// Two sides need it: the text renderer, which fetches a glyph per code point,
// and the text editor, which computes in code points (an umlaut = one step).
// ICU carries the Unicode work.

// Append the code points of the half-open UTF-16 range [begin, end) of an
// ICU string to out. char32At returns the full code point at a unit index; a
// code point above the BMP occupies two UTF-16 units, hence the variable step.
void AppendCodePoints(const icu::UnicodeString& text, std::int32_t begin,
                      std::int32_t end, std::vector<char32_t>& out);

// Decode UTF-8 into a sequence of Unicode code points ready for glyph lookup.
//
// Strings reaching the renderer are UTF-8 (strftime month names, which carry
// accents in many locales, or user-entered title text). ICU does the heavy
// lifting:
//   1. fromUTF8 turns the bytes into a UTF-16 string, replacing malformed
//      sequences with U+FFFD instead of producing stray glyphs.
//   2. NFC normalisation folds a base letter plus a combining mark (e + ◌́)
//      into its precomposed form (é) where one exists, so it renders as a
//      single glyph — FreeType has no shaping engine of its own.
//   3. A grapheme BreakIterator walks user-perceived characters; the code
//      points of each cluster are emitted in order. Code points that have no
//      precomposed form (and full complex-script shaping, e.g. Arabic/Indic)
//      would need HarfBuzz and are out of scope; they fall back to per-code
//      point glyphs, and a code point absent from the face renders as the
//      font's .notdef glyph.
[[nodiscard]] std::vector<char32_t> DecodeUtf8(const std::string& text);

// The counterpart to DecodeUtf8 for the way back: the text editor computes in
// code points, while what gets stored and drawn is UTF-8.
[[nodiscard]] std::string EncodeUtf8(const std::u32string& code_points);

#endif  // UTF8_CODEC_HPP
