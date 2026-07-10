#ifndef FREETYPE_HPP
#define FREETYPE_HPP

// Use the canonical FreeType entry point. <ft2build.h> defines the
// configuration macros (FT_FREETYPE_H, ...) that resolve to the correct
// internal headers; this insulates us from FreeType's header layout, which has
// moved between versions. Including the internal paths
// (<freetype2/freetype/freetype.h>, ...) directly only worked because the
// include path happened to contain freetype2/ and is not portable. The base
// API header alone is sufficient here — it transitively pulls in ftimage.h
// (FT_GLYPH_FORMAT_BITMAP) and fterrors.h (FT_Error_String); the glyph/image
// sub-APIs are not used.
// FT_FREETYPE_H is a macro defined by ft2build.h, so it can only be included
// after it. clang-tidy's include sorter would order FT_FREETYPE_H first
// (case-sensitive, brackets stripped), which breaks the build; FreeType's
// contract has no in-code fix for this.
// NOLINTNEXTLINE(llvm-include-order)
#include <ft2build.h>
#include FT_FREETYPE_H

#endif  // FREETYPE_HPP
