#ifndef UNICODE_HPP
#define UNICODE_HPP

// Single entry point for the ICU headers used by the text renderer, mirroring
// freetype.hpp. ICU (https://icu.unicode.org/) provides the Unicode handling in
// font.hpp: UTF-8 decoding, NFC normalisation, and grapheme segmentation.
//   - utypes.h      — UErrorCode, UChar32, the U_SUCCESS/U_FAILURE macros
//   - unistr.h      — icu::UnicodeString
//   - stringpiece.h — icu::StringPiece (UnicodeString::fromUTF8 argument)
//   - normalizer2.h — icu::Normalizer2 (NFC)
//   - brkiter.h     — icu::BreakIterator (grapheme clusters)
//   - locid.h       — icu::Locale
#include <unicode/brkiter.h>
#include <unicode/locid.h>
#include <unicode/normalizer2.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

#endif  // UNICODE_HPP
