#ifndef UNICODE_HPP
#define UNICODE_HPP

// Single entry point for the ICU headers used by the text renderer, mirroring
// freetype.hpp. ICU (https://icu.unicode.org/) provides the Unicode handling in
// font.hpp: UTF-8 decoding, NFC normalisation, and grapheme segmentation.
//   - utypes.h      — UErrorCode, the U_SUCCESS/U_FAILURE macros
//   - umachine.h    — UChar32
//   - unistr.h      — icu::UnicodeString
//   - stringpiece.h — icu::StringPiece (UnicodeString::fromUTF8 argument)
//   - normalizer2.h — icu::Normalizer2 (NFC)
//   - brkiter.h     — icu::BreakIterator (grapheme clusters)
//   - locid.h       — icu::Locale
//
// The export pragmas say that this is the entry point: whoever includes this
// header has included the ICU headers below, and the include check asks for no
// second spelling of them.
#include <unicode/brkiter.h>      // IWYU pragma: export
#include <unicode/locid.h>        // IWYU pragma: export
#include <unicode/normalizer2.h>  // IWYU pragma: export
#include <unicode/stringpiece.h>  // IWYU pragma: export
#include <unicode/umachine.h>     // IWYU pragma: export
#include <unicode/unistr.h>       // IWYU pragma: export
#include <unicode/utypes.h>       // IWYU pragma: export

#endif  // UNICODE_HPP
