/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "graphics_engine.hpp"
#include "mvp_matrices.hpp"
#include "render_to_texture.hpp"

#include <glad/glad.h>

#include <algorithm>
// #include <iostream>
#include <memory>
#include <string>

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
  std::array<GLsizei, 2> viewport;
  rectf dimension;
  std::vector<unsigned char> image;
};

class ImageComposer {
public:
  ImageComposer(size_t width, size_t height, const rectf &ortho_dimension,
                std::shared_ptr<GraphicsEngine> graphics_engine, int msaa_samples)
      : width(width), height(height), ortho_dimension(ortho_dimension),
        graphics_engine(graphics_engine), number_width_parts(0), number_height_parts(0),
        pixel_size(0), standard_size(0), width_remainder(0), height_remainder(0),
        width_ortho_standard(0), height_ortho_standard(0), width_ortho_remainder(0),
        height_ortho_remainder(0), msaa_samples(msaa_samples)
  {
    pixel_size = 4 * sizeof(unsigned char);

    standard_size = GetStandardTextureSize();

    CalculatePixelRemainder();
    CalculateOrthoStandard();
    CalculateOrthoRemainder();
    CalculateNumberImageParts();

    ConfigureImageParts();
    RenderImageParts();
    ComposeImageParts();
    VerticalFlip();
  }

  ImagePart &ImagePartRef(size_t x, size_t y) { return image_parts[x + y * number_width_parts]; }

  std::vector<unsigned char> CopyImage() const { return image; }

  size_t NumberImageParts() const { return image_parts.size(); }

  size_t NumberWidthParts() const { return number_width_parts; }

  size_t NumberHeightParts() const { return number_height_parts; }

private:
  int GetStandardTextureSize()
  {
    // int max_texture_size = 0;
    // glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    int standard_size = 4096;
    return standard_size;
  }

  void CalculatePixelRemainder()
  {
    width_remainder = width % standard_size;
    height_remainder = height % standard_size;
  }

  void CalculateOrthoStandard()
  {
    float width_ratio = 1.f;
    float height_ratio = 1.f;

    if (width > standard_size) {
      width_ratio = static_cast<float>(standard_size) / static_cast<float>(width);
    }

    if (height > standard_size) {
      height_ratio = static_cast<float>(standard_size) / static_cast<float>(height);
    }

    width_ortho_standard = ortho_dimension.width() * width_ratio;
    height_ortho_standard = ortho_dimension.height() * height_ratio;
  }

  void CalculateOrthoRemainder()
  {
    float width_remainder_ratio = static_cast<float>(width_remainder) / static_cast<float>(width);
    float height_remainder_ratio =
        static_cast<float>(height_remainder) / static_cast<float>(height);

    width_ortho_remainder = ortho_dimension.width() * width_remainder_ratio;
    height_ortho_remainder = ortho_dimension.height() * height_remainder_ratio;
  }

  void CalculateNumberImageParts()
  {
    size_t number_width_parts_without_remainder = width / standard_size;
    size_t number_height_parts_without_remainder = height / standard_size;

    number_width_parts = number_width_parts_without_remainder + 1;
    number_height_parts = number_height_parts_without_remainder + 1;

    image_parts.resize(number_width_parts * number_height_parts);
  }

  void ConfigureImageParts()
  {
    for (size_t y_index = 0; y_index < number_height_parts; ++y_index) {
      for (size_t x_index = 0; x_index < number_width_parts; ++x_index) {
        int current_width = 0;
        int current_height = 0;

        float x_index_float = static_cast<float>(x_index);
        float y_index_float = static_cast<float>(y_index);

        rectf current_ortho_part;

        if (x_index == number_width_parts - 1) {
          current_width = width_remainder;
          current_ortho_part.setL(ortho_dimension.l() + x_index_float * width_ortho_standard);
          current_ortho_part.setR(current_ortho_part.l() + width_ortho_remainder);
        } else {
          current_width = standard_size;
          current_ortho_part.setL(ortho_dimension.l() + x_index_float * width_ortho_standard);
          current_ortho_part.setR(current_ortho_part.l() + width_ortho_standard);
        }

        if (y_index == number_height_parts - 1) {
          current_height = height_remainder;
          current_ortho_part.setB(ortho_dimension.b() + y_index_float * height_ortho_standard);
          current_ortho_part.setT(current_ortho_part.b() + height_ortho_remainder);
        } else {
          current_height = standard_size;
          current_ortho_part.setB(ortho_dimension.b() + y_index_float * height_ortho_standard);
          current_ortho_part.setT(current_ortho_part.b() + height_ortho_standard);
        }

        ImagePartRef(x_index, y_index).viewport[0] = current_width;
        ImagePartRef(x_index, y_index).viewport[1] = current_height;
        ImagePartRef(x_index, y_index).dimension = current_ortho_part;
      }
    }
  }

  void RenderImageParts()
  {
    MVP mvp;
    mvp.SetView(glm::translate(glm::vec3(0.f, 0.f, 0.f)));

    for (size_t y_index = 0; y_index < NumberHeightParts(); ++y_index) {
      for (size_t x_index = 0; x_index < NumberWidthParts(); ++x_index) {
        auto &current_image_part = ImagePartRef(x_index, y_index);

        RenderToTexture render_texture(current_image_part.viewport[0],
                                       current_image_part.viewport[1], msaa_samples);

        render_texture.BeginRender();
        mvp.SetProjection(
            glm::ortho(current_image_part.dimension.l(), current_image_part.dimension.r(),
                       current_image_part.dimension.b(), current_image_part.dimension.t()));

        graphics_engine->SetMVP(mvp);
        graphics_engine->Render();

        render_texture.EndRender();

        ImagePartRef(x_index, y_index).image = render_texture.CopyImage();
      }
    }
  }

  void ComposeImageParts()
  {
    image.resize(pixel_size * width * height);

    size_t current_width_position = 0;
    size_t current_height_position = 0;

    for (size_t y_index = 0; y_index < NumberHeightParts(); ++y_index) {
      for (size_t x_index = 0; x_index < NumberWidthParts(); ++x_index) {
        auto &current_image_part = ImagePartRef(x_index, y_index);

        auto current_image_position =
            current_height_position * width * pixel_size + current_width_position * pixel_size;

        for (size_t index = 0; index < current_image_part.viewport[1]; ++index) {
          size_t x_size = static_cast<size_t>(current_image_part.viewport[0]) * pixel_size;
          size_t current_x_begin = index * x_size;
          size_t current_x_end = current_x_begin + x_size;

          std::copy(current_image_part.image.cbegin() + current_x_begin,
                    current_image_part.image.cbegin() + current_x_end,
                    image.begin() + current_image_position + index * width * pixel_size);
        }

        current_width_position += current_image_part.viewport[0];
      }

      auto &current_image_part = ImagePartRef(0, y_index);
      current_width_position = 0;
      current_height_position += current_image_part.viewport[1];
    }
  }

  void VerticalFlip()
  {
    std::vector<unsigned char> image_copy(image);

    for (size_t index = 0; index < height; ++index) {
      size_t row_size = static_cast<size_t>(width) * pixel_size;
      size_t current_row_begin = index * row_size;
      size_t current_row_end = current_row_begin + row_size;

      std::copy(image_copy.cbegin() + current_row_begin, image_copy.cbegin() + current_row_end,
                image.end() - current_row_end);
    }
  }

  size_t width;
  size_t height;

  rectf ortho_dimension;

  std::vector<ImagePart> image_parts;
  size_t number_width_parts;
  size_t number_height_parts;

  std::shared_ptr<GraphicsEngine> graphics_engine;

  size_t pixel_size;
  std::vector<unsigned char> image;

  int standard_size;
  int width_remainder;
  int height_remainder;
  float width_ortho_standard;
  float height_ortho_standard;
  float width_ortho_remainder;
  float height_ortho_remainder;
  const int msaa_samples;
};

class RenderToPNG {
public:
  RenderToPNG(const std::string &file_path, const rectf &image_dimension, const float dpi,
              std::shared_ptr<GraphicsEngine> graphics_engine, int msaa_samples)
      : file_path(file_path), ortho_dimensions(image_dimension), dpi(dpi), image_width(0),
        image_height(0), graphics_engine(graphics_engine), msaa_samples(msaa_samples)
  {
    RenderPicture();
  }

  void RenderPicture()
  {
    // calculate the rounded pixel dimension from dpi
    const float dots_per_millimeter = dots_per_inch_to_dots_per_millimeter(dpi);
    const float pixel_width_float = std::round(ortho_dimensions.width() * dots_per_millimeter);
    const float pixel_height_float = std::round(ortho_dimensions.height() * dots_per_millimeter);

    image_width = static_cast<int>(pixel_width_float);
    image_height = static_cast<int>(pixel_height_float);

    float real_width_dpm = pixel_width_float / ortho_dimensions.width();
    float real_height_dpm = pixel_height_float / ortho_dimensions.height();

    float real_width_dpi = dots_per_millimeter_to_dots_per_inch(real_width_dpm);
    float real_height_dpi = dots_per_millimeter_to_dots_per_inch(real_height_dpm);

    auto max_height = MaxPngHeight();
    auto max_height_from_width = MaxPngWidthForHeight(image_width, image_height);
    auto max_width_from_height = MaxPngHeightForWidth(image_width, image_height);

    bool image_too_large = false;
    if (image_height > max_height || image_height > max_height_from_width) {
      image_too_large = true;
    }

    if (image_width > max_width_from_height) {
      image_too_large = true;
    }

    ImageComposer image(image_width, image_height, ortho_dimensions, graphics_engine, msaa_samples);
    auto image_data = image.CopyImage();
    SavePicture(file_path.c_str(), image_data, image_width, image_height);
  }

private:
  size_t MaxPngHeight() { return PNG_UINT_32_MAX / (sizeof(png_bytep)); }

  size_t MaxPngWidthForHeight(size_t width, size_t height)
  {
    const size_t bytes_per_pixel = 4;
    const size_t max_height = PNG_SIZE_MAX / (width * bytes_per_pixel);
    return max_height;
  }

  size_t MaxPngHeightForWidth(size_t width, size_t height)
  {
    const size_t bytes_per_pixel = 4;
    const size_t max_width = PNG_SIZE_MAX / (height * bytes_per_pixel);
    return max_width;
  }

  size_t MaxPngSize() const { return PNG_SIZE_MAX; }

  void SavePicture(const char *file_name, std::vector<unsigned char> &image, size_t width,
                   size_t height)
  {
    // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

    FILE *fp = nullptr;
    auto file_error = fopen_s(&fp, file_name, "wb");
    if (fp == NULL || file_error) {
    }

    png_structp png_ptr = nullptr;
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
      // std::cout << "png_create_write_struct failed!" << '\n';
    }

    png_infop info_ptr = nullptr;
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
      // std::cout << "png_create_info_struct failed!" << '\n';
      fclose(fp);
      png_destroy_write_struct(&png_ptr, NULL);
    }

    auto setjmp_value = setjmp(png_jmpbuf(png_ptr));
    if (setjmp_value) {
      // std::cout << "setjmp(png_jmpbuf(png_ptr)) failed!" << '\n';
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);

    const size_t bytes_per_pixel = 4;

    std::vector<png_bytep> row_pointers(height);

    for (size_t index = 0; index < height; ++index) {
      png_bytep current_row_ptr = image.data() + index * width * bytes_per_pixel;
      row_pointers[index] = current_row_ptr;
    }

    // png_write_image(png_ptr, row_pointers);

    for (size_t index = 0; index < height; ++index) {
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

  float dots_per_inch_to_dots_per_millimeter(const float dpi) const
  {
    constexpr float ratio = 1.0f / 25.4f;
    const float dots_per_millimeter = ratio * dpi;
    return dots_per_millimeter;
  }

  float dots_per_millimeter_to_dots_per_inch(const float dpm) const
  {
    constexpr float ratio = 25.4f;
    const float dots_per_inch = ratio * dpm;
    return dots_per_inch;
  }

  size_t image_width;
  size_t image_height;

  const std::string file_path;
  const rectf ortho_dimensions;
  const float dpi;
  const int msaa_samples;

  std::shared_ptr<GraphicsEngine> graphics_engine;
};
