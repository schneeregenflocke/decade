#include "png_writer.hpp"

#include <cstddef>
#include <cstdio>
#include <gsl/pointers>
#include <memory>
#include <vector>

extern "C" {
#include <png.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
}

namespace png_io {
namespace {

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
[[nodiscard]] gsl::owner<std::FILE*> OpenForWrite(const char* file_name) {
  gsl::owner<std::FILE*> file = nullptr;
#ifdef _MSC_VER
  (void)fopen_s(&file, file_name, "wb");
#else
  file = std::fopen(file_name, "wb");
#endif  // _MSC_VER
  return file;
}

}  // namespace

bool FitsPngLimits(std::size_t width, std::size_t height) {
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

bool WriteRgbaPng(const char* file_name, std::vector<unsigned char>& pixels,
                  PngImageSize size) {
  // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

  const UniqueFile file(OpenForWrite(file_name));
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
  // to exactly these two checks (see AGENTS.md, "Warnings, the clang-tidy and
  // the sanitizer gate").
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
