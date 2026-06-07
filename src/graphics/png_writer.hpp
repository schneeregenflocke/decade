#ifndef PNG_WRITER_HPP
#define PNG_WRITER_HPP

#include <cstddef>
#include <cstdio>
#include <vector>

extern "C" {
#include <png.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
}

// Infrastructure: writes RGBA8 pixel buffers to PNG files via libpng.
//
// All of libpng's C API lives behind this header. That matters because libpng
// reports errors through setjmp/longjmp, and a longjmp does *not* run C++
// destructors as it unwinds — mixing it with RAII objects is a known footgun.
// Confining it here keeps the rest of the codebase free of both the C API and
// the setjmp control flow, so no caller has to reason about that hazard.
namespace png_io {

struct PngImageSize {
  std::size_t width{0};
  std::size_t height{0};
};

inline constexpr std::size_t kBytesPerPixel = 4;

// True if a width x height RGBA image stays within PNG's format and addressing
// limits. Anything larger cannot be written and must be rejected by the caller.
[[nodiscard]] inline bool FitsPngLimits(std::size_t width, std::size_t height) {
  if (width == 0 || height == 0) {
    return false;
  }
  const std::size_t max_height_by_format = PNG_UINT_32_MAX / sizeof(png_bytep);
  const std::size_t max_height_by_address =
      PNG_SIZE_MAX / (width * kBytesPerPixel);
  const std::size_t max_width_by_address =
      PNG_SIZE_MAX / (height * kBytesPerPixel);
  return height <= max_height_by_format && height <= max_height_by_address &&
         width <= max_width_by_address;
}

// Writes the RGBA8 `pixels` (row-major, top-left origin) to `file_name`.
// Returns false if the file cannot be opened or libpng signals an error.
//
// The NOLINT block covers the unavoidable C idioms of libpng: raw FILE*
// ownership, setjmp/longjmp error handling, and pointer arithmetic over the
// pixel buffer. This header is the single place they are allowed to live; see
// the file-level comment above.
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cert-err33-c,cert-err52-cpp,modernize-avoid-setjmp-longjmp,cppcoreguidelines-pro-bounds-pointer-arithmetic)
inline bool WriteRgbaPng(const char* file_name,
                         std::vector<unsigned char>& pixels,
                         PngImageSize size) {
  // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

  FILE* file = nullptr;
#ifdef _MSC_VER
  const auto open_error = fopen_s(&file, file_name, "wb");
  if (file == nullptr || open_error) {
    return false;
  }
#else
  file = std::fopen(file_name, "wb");
  if (file == nullptr) {
    return false;
  }
#endif  // _MSC_VER

  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (png == nullptr) {
    std::fclose(file);
    return false;
  }

  png_infop info = png_create_info_struct(png);
  if (info == nullptr) {
    std::fclose(file);
    png_destroy_write_struct(&png, nullptr);
    return false;
  }

  if (setjmp(png_jmpbuf(png)) != 0) {
    std::fclose(file);
    png_destroy_write_struct(&png, &info);
    return false;
  }

  png_init_io(png, file);

  constexpr int kBitDepth = 8;  // bits per RGBA channel
  png_set_IHDR(png, info, static_cast<png_uint_32>(size.width),
               static_cast<png_uint_32>(size.height), kBitDepth,
               PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png, info);

  std::vector<png_bytep> row_pointers(size.height);
  for (std::size_t row = 0; row < size.height; ++row) {
    row_pointers[row] = pixels.data() + (row * size.width * kBytesPerPixel);
  }
  for (std::size_t row = 0; row < size.height; ++row) {
    png_bytep row_ptr = row_pointers[row];
    png_write_rows(png, &row_ptr, 1);
  }

  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  std::fclose(file);
  return true;
}
// NOLINTEND(cppcoreguidelines-owning-memory,cert-err33-c,cert-err52-cpp,modernize-avoid-setjmp-longjmp,cppcoreguidelines-pro-bounds-pointer-arithmetic)

}  // namespace png_io

#endif  // PNG_WRITER_HPP
