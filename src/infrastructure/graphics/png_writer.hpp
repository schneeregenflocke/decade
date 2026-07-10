#ifndef PNG_WRITER_HPP
#define PNG_WRITER_HPP

#include <cstddef>
#include <cstdio>
#include <gsl/pointers>
#include <memory>
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
// All of libpng's C API lives behind this header. libpng reports fatal errors
// through setjmp/longjmp, and a longjmp does *not* run C++ destructors as it
// unwinds — per [csetjmp] it is undefined behaviour if the jump would skip a
// non-trivial destructor. We respect that by constructing every object with a
// non-trivial destructor (the FILE owner, the row-pointer vector) *before* the
// setjmp, so the jump can never bypass their cleanup. Raw stdio ownership is
// expressed with gsl::owner so the static analysis can verify it. Confining
// libpng to this header keeps the rest of the codebase free of the C API.
namespace png_io {

struct PngImageSize {
  std::size_t width{0};
  std::size_t height{0};
};

inline constexpr std::size_t kBytesPerPixel = 4;

namespace detail {

// Deleter that lets a unique_ptr own a C FILE. Taking a gsl::owner makes the
// ownership transfer into fclose explicit for the analyser.
struct FileCloser {
  void operator()(gsl::owner<std::FILE*> file) const noexcept {
    (void)std::fclose(file);
  }
};

using UniqueFile = std::unique_ptr<std::FILE, FileCloser>;

// Opens `file_name` for binary writing. Returns an owning handle (null on
// failure) that the caller hands straight to a UniqueFile.
[[nodiscard]] inline gsl::owner<std::FILE*> OpenForWrite(
    const char* file_name) {
  gsl::owner<std::FILE*> file = nullptr;
#ifdef _MSC_VER
  (void)fopen_s(&file, file_name, "wb");
#else
  file = std::fopen(file_name, "wb");
#endif  // _MSC_VER
  return file;
}

}  // namespace detail

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
inline bool WriteRgbaPng(const char* file_name,
                         std::vector<unsigned char>& pixels,
                         PngImageSize size) {
  // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

  const detail::UniqueFile file(detail::OpenForWrite(file_name));
  if (!file) {
    return false;
  }

  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (png == nullptr) {
    return false;
  }

  png_infop info = png_create_info_struct(png);
  if (info == nullptr) {
    png_destroy_write_struct(&png, nullptr);
    return false;
  }

  // Built before the setjmp: row pointers into `pixels`, indexed (not pointer
  // arithmetic) so the bounds-safety check is satisfied and the vector's
  // destructor is never skipped by a longjmp.
  std::vector<png_bytep> row_pointers(size.height);
  for (std::size_t row = 0; row < size.height; ++row) {
    row_pointers[row] = &pixels[row * size.width * kBytesPerPixel];
  }

  // The one construct libpng's contract forces on us and that has no in-code
  // fix: its default error handler longjmps back here. Suppression is scoped
  // to exactly these two checks (see "Build hygiene" in CLAUDE.md).
  // NOLINTNEXTLINE(cert-err52-cpp,modernize-avoid-setjmp-longjmp)
  if (setjmp(png_jmpbuf(png)) != 0) {
    png_destroy_write_struct(&png, &info);
    return false;
  }

  png_init_io(png, file.get());

  constexpr int kBitDepth = 8;  // bits per RGBA channel
  png_set_IHDR(png, info, static_cast<png_uint_32>(size.width),
               static_cast<png_uint_32>(size.height), kBitDepth,
               PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png, info);

  for (std::size_t row = 0; row < size.height; ++row) {
    png_write_rows(png, &row_pointers[row], 1);
  }

  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  return true;
}

}  // namespace png_io

#endif  // PNG_WRITER_HPP
