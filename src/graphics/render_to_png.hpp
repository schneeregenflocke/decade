#ifndef HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_RENDER_TO_PNG_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_RENDER_TO_PNG_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "graphics_engine.hpp"
#include "mvp_matrices.hpp"
#include "render_to_texture.hpp"
// #include <iostream>
#include <memory>
#include <string>
#include <utility>

extern "C" {
#include <png.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
}

struct ImagePart {
  std::array<GLsizei, 2> viewport{0, 0};
  rectf dimension;
  std::vector<unsigned char> image;
};

struct ImageSize {
  size_t width{0};
  size_t height{0};
};

class ImageComposer {
 public:
  ImageComposer(ImageSize image_size, const rectf& ortho_dimension_in,
                const std::shared_ptr<GraphicsEngine>& graphics_engine_in,
                int msaa_samples_in)
      : width(image_size.width),
        height(image_size.height),
        ortho_dimension(ortho_dimension_in),
        graphics_engine(graphics_engine_in),
        pixel_size(kBytesPerPixel),
        standard_size(kStandardTextureSize),
        msaa_samples(msaa_samples_in) {
    CalculatePixelRemainder();
    CalculateOrthoStandard();
    CalculateOrthoRemainder();
    CalculateNumberImageParts();

    ConfigureImageParts();
    RenderImageParts();
    ComposeImageParts();
    VerticalFlip();
  }

  ImagePart& ImagePartRef(size_t x_index, size_t y_index) {
    return image_parts[x_index + (y_index * number_width_parts)];
  }

  [[nodiscard]] std::vector<unsigned char> CopyImage() const { return image; }

  [[nodiscard]] size_t NumberImageParts() const { return image_parts.size(); }

  [[nodiscard]] size_t NumberWidthParts() const { return number_width_parts; }

  [[nodiscard]] size_t NumberHeightParts() const { return number_height_parts; }

 private:
  void CalculatePixelRemainder() {
    width_remainder = width % standard_size;
    height_remainder = height % standard_size;
  }

  void CalculateOrthoStandard() {
    constexpr float kOne = 1.0F;
    float width_ratio = kOne;
    float height_ratio = kOne;

    if (width > standard_size) {
      width_ratio =
          static_cast<float>(standard_size) / static_cast<float>(width);
    }

    if (height > standard_size) {
      height_ratio =
          static_cast<float>(standard_size) / static_cast<float>(height);
    }

    width_ortho_standard = ortho_dimension.width() * width_ratio;
    height_ortho_standard = ortho_dimension.height() * height_ratio;
  }

  void CalculateOrthoRemainder() {
    const float width_remainder_ratio =
        static_cast<float>(width_remainder) / static_cast<float>(width);
    const float height_remainder_ratio =
        static_cast<float>(height_remainder) / static_cast<float>(height);

    width_ortho_remainder = ortho_dimension.width() * width_remainder_ratio;
    height_ortho_remainder = ortho_dimension.height() * height_remainder_ratio;
  }

  void CalculateNumberImageParts() {
    const size_t number_width_parts_without_remainder = width / standard_size;
    const size_t number_height_parts_without_remainder = height / standard_size;

    number_width_parts =
        number_width_parts_without_remainder + (width_remainder > 0 ? 1 : 0);
    number_height_parts =
        number_height_parts_without_remainder + (height_remainder > 0 ? 1 : 0);

    image_parts.resize(number_width_parts * number_height_parts);
  }

  void ConfigureImageParts() {
    const bool width_has_remainder = width_remainder > 0;
    const bool height_has_remainder = height_remainder > 0;

    for (size_t y_index = 0; y_index < number_height_parts; ++y_index) {
      for (size_t x_index = 0; x_index < number_width_parts; ++x_index) {
        GLsizei current_width = 0;
        GLsizei current_height = 0;

        const auto x_index_float = static_cast<float>(x_index);
        const auto y_index_float = static_cast<float>(y_index);

        rectf current_ortho_part;

        if ((x_index == number_width_parts - 1) && width_has_remainder) {
          current_width = static_cast<GLsizei>(width_remainder);
          current_ortho_part.setL(ortho_dimension.l() +
                                  (x_index_float * width_ortho_standard));
          current_ortho_part.setR(current_ortho_part.l() +
                                  width_ortho_remainder);
        } else {
          current_width = static_cast<GLsizei>(standard_size);
          current_ortho_part.setL(ortho_dimension.l() +
                                  (x_index_float * width_ortho_standard));
          current_ortho_part.setR(current_ortho_part.l() +
                                  width_ortho_standard);
        }

        if ((y_index == number_height_parts - 1) && height_has_remainder) {
          current_height = static_cast<GLsizei>(height_remainder);
          current_ortho_part.setB(ortho_dimension.b() +
                                  (y_index_float * height_ortho_standard));
          current_ortho_part.setT(current_ortho_part.b() +
                                  height_ortho_remainder);
        } else {
          current_height = static_cast<GLsizei>(standard_size);
          current_ortho_part.setB(ortho_dimension.b() +
                                  (y_index_float * height_ortho_standard));
          current_ortho_part.setT(current_ortho_part.b() +
                                  height_ortho_standard);
        }

        ImagePartRef(x_index, y_index).viewport[0] = current_width;
        ImagePartRef(x_index, y_index).viewport[1] = current_height;
        ImagePartRef(x_index, y_index).dimension = current_ortho_part;
      }
    }
  }

  void RenderImageParts() {
    MVP mvp;
    mvp.SetView(glm::translate(glm::vec3(0.0F, 0.0F, 0.0F)));

    for (size_t y_index = 0; y_index < NumberHeightParts(); ++y_index) {
      for (size_t x_index = 0; x_index < NumberWidthParts(); ++x_index) {
        auto& current_image_part = ImagePartRef(x_index, y_index);

        RenderToTexture render_texture(current_image_part.viewport[0],
                                       current_image_part.viewport[1],
                                       msaa_samples);

        render_texture.BeginRender();
        mvp.SetProjection(glm::ortho(current_image_part.dimension.l(),
                                     current_image_part.dimension.r(),
                                     current_image_part.dimension.b(),
                                     current_image_part.dimension.t()));

        graphics_engine->SetMVP(mvp);
        graphics_engine->Render();

        render_texture.EndRender();

        ImagePartRef(x_index, y_index).image = render_texture.CopyImage();
      }
    }
  }

  void ComposeImageParts() {
    image.resize(pixel_size * width * height);

    size_t current_width_position = 0;
    size_t current_height_position = 0;

    for (size_t y_index = 0; y_index < NumberHeightParts(); ++y_index) {
      for (size_t x_index = 0; x_index < NumberWidthParts(); ++x_index) {
        auto& current_image_part = ImagePartRef(x_index, y_index);

        const size_t current_image_position =
            (current_height_position * width * pixel_size) +
            (current_width_position * pixel_size);

        const auto viewport_height =
            static_cast<size_t>(current_image_part.viewport[1]);
        for (size_t index = 0; index < viewport_height; ++index) {
          const size_t x_size =
              static_cast<size_t>(current_image_part.viewport[0]) * pixel_size;
          const size_t current_x_begin = index * x_size;
          const size_t current_x_end = current_x_begin + x_size;

          const auto image_begin_offset = static_cast<std::ptrdiff_t>(
              current_image_position + (index * width * pixel_size));
          const auto current_x_begin_offset =
              static_cast<std::ptrdiff_t>(current_x_begin);
          const auto current_x_end_offset =
              static_cast<std::ptrdiff_t>(current_x_end);

          std::copy(current_image_part.image.cbegin() + current_x_begin_offset,
                    current_image_part.image.cbegin() + current_x_end_offset,
                    image.begin() + image_begin_offset);
        }

        current_width_position +=
            static_cast<std::size_t>(current_image_part.viewport[0]);
      }

      auto& current_image_part = ImagePartRef(0, y_index);
      current_width_position = 0;
      current_height_position +=
          static_cast<std::size_t>(current_image_part.viewport[1]);
    }
  }

  void VerticalFlip() {
    const std::vector<unsigned char> image_copy(image);

    for (size_t index = 0; index < height; ++index) {
      const size_t row_size = width * pixel_size;
      const size_t current_row_begin = index * row_size;
      const size_t current_row_end = current_row_begin + row_size;

      const auto row_begin_offset =
          static_cast<std::ptrdiff_t>(current_row_begin);
      const auto row_end_offset = static_cast<std::ptrdiff_t>(current_row_end);
      std::copy(image_copy.cbegin() + row_begin_offset,
                image_copy.cbegin() + row_end_offset,
                image.end() - row_end_offset);
    }
  }

  size_t width{0};
  size_t height{0};

  rectf ortho_dimension;

  std::vector<ImagePart> image_parts;
  size_t number_width_parts{0};
  size_t number_height_parts{0};

  std::shared_ptr<GraphicsEngine> graphics_engine;

  size_t pixel_size{0};
  std::vector<unsigned char> image;

  size_t standard_size{0};
  size_t width_remainder{0};
  size_t height_remainder{0};
  float width_ortho_standard{0.0F};
  float height_ortho_standard{0.0F};
  float width_ortho_remainder{0.0F};
  float height_ortho_remainder{0.0F};
  const int msaa_samples;

  static constexpr size_t kBytesPerPixel = 4;
  static constexpr size_t kStandardTextureSize = 4096;
};

class RenderToPNG {
 public:
  RenderToPNG(std::string file_path_in, const rectf& image_dimension,
              const float dpi_in,
              std::shared_ptr<GraphicsEngine> graphics_engine_in,
              int msaa_samples_in)
      : file_path(std::move(file_path_in)),
        ortho_dimensions(image_dimension),
        dpi(dpi_in),

        graphics_engine(std::move(graphics_engine_in)),
        msaa_samples(msaa_samples_in) {
    RenderPicture();
  }

  void RenderPicture() {
    // calculate the rounded pixel dimension from dpi
    const float dots_per_millimeter = dots_per_inch_to_dots_per_millimeter(dpi);
    const float pixel_width_float =
        std::round(ortho_dimensions.width() * dots_per_millimeter);
    const float pixel_height_float =
        std::round(ortho_dimensions.height() * dots_per_millimeter);

    image_width = static_cast<size_t>(std::max(0.0F, pixel_width_float));
    image_height = static_cast<size_t>(std::max(0.0F, pixel_height_float));

    if (image_width == 0 || image_height == 0) {
      return;
    }

    const auto max_height = MaxPngHeight();
    const auto max_height_from_width = MaxPngHeightForWidth(image_width);
    const auto max_width_from_height = MaxPngWidthForHeight(image_height);

    const bool image_too_large = (image_height > max_height) ||
                                 (image_height > max_height_from_width) ||
                                 (image_width > max_width_from_height);
    if (image_too_large) {
      return;
    }

    const ImageComposer image(
        ImageSize{.width = image_width, .height = image_height},
        ortho_dimensions, graphics_engine, msaa_samples);
    auto image_data = image.CopyImage();
    SavePicture(file_path.c_str(), image_data,
                PngSize{.width = image_width, .height = image_height});
  }

 public:
  struct PngSize {
    size_t width{0};
    size_t height{0};
  };

  static void SaveRgbaPng(const char* file_name,
                          std::vector<unsigned char>& image, size_t width,
                          size_t height) {
    SavePicture(file_name, image, PngSize{.width = width, .height = height});
  }

 private:
  static size_t MaxPngHeight() { return PNG_UINT_32_MAX / (sizeof(png_bytep)); }

  static size_t MaxPngWidthForHeight(size_t height) {
    const size_t max_width = PNG_SIZE_MAX / (height * kBytesPerPixel);
    return max_width;
  }

  static size_t MaxPngHeightForWidth(size_t width) {
    const size_t max_height = PNG_SIZE_MAX / (width * kBytesPerPixel);
    return max_height;
  }

  static void SavePicture(const char* file_name,
                          std::vector<unsigned char>& image, PngSize size) {
    // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

    FILE* fp = nullptr;
#if defined(_MSC_VER)
    auto file_error = fopen_s(&fp, file_name, "wb");
    if (fp == nullptr || file_error) {
      return;
    }
#else
    fp = std::fopen(file_name, "wb");
    if (fp == nullptr) {
      return;
    }
#endif  // _MSC_VER

    png_structp png_ptr = nullptr;
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr,
                                      nullptr);
    if (png_ptr == nullptr) {
      // std::cout << "png_create_write_struct failed!" << '\n';
      fclose(fp);
      return;
    }

    png_infop info_ptr = nullptr;
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
      // std::cout << "png_create_info_struct failed!" << '\n';
      fclose(fp);
      png_destroy_write_struct(&png_ptr, nullptr);
      return;
    }

    auto setjmp_value = setjmp(png_jmpbuf(png_ptr));
    if (setjmp_value) {
      // std::cout << "setjmp(png_jmpbuf(png_ptr)) failed!" << '\n';
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return;
    }

    png_init_io(png_ptr, fp);

    const auto png_width = static_cast<png_uint_32>(size.width);
    const auto png_height = static_cast<png_uint_32>(size.height);
    png_set_IHDR(png_ptr, info_ptr, png_width, png_height, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    std::vector<png_bytep> row_pointers(size.height);

    for (size_t index = 0; index < size.height; ++index) {
      png_bytep current_row_ptr =
          image.data() +
          index * size.width * static_cast<size_t>(kBytesPerPixel);
      row_pointers[index] = current_row_ptr;
    }

    // png_write_image(png_ptr, row_pointers);

    for (size_t index = 0; index < size.height; ++index) {
      // printf("write row %zu of %zu\n", index, height);
      png_bytep current_row_ptr = row_pointers[index];
      png_write_rows(png_ptr, &current_row_ptr, 1);
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    if (fp) {
      fclose(fp);
    }
  }

  [[nodiscard]] float dots_per_inch_to_dots_per_millimeter(
      const float dpi_value) const {
    constexpr float ratio = 1.0F / 25.4F;
    const float dots_per_millimeter = ratio * dpi_value;
    return dots_per_millimeter;
  }

  const std::string file_path;
  const rectf ortho_dimensions;
  const float dpi;
  size_t image_width{0};
  size_t image_height{0};
  std::shared_ptr<GraphicsEngine> graphics_engine;
  int msaa_samples;

  static constexpr size_t kBytesPerPixel = 4;
};
#endif  // HOME_TITAN99_CODE_DECADE_SRC_GRAPHICS_RENDER_TO_PNG_HPP
