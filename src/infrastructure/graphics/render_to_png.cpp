#include "render_to_png.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "../../common/debug_log.hpp"
#include "graphics_engine.hpp"
#include "mvp_matrices.hpp"
#include "png_writer.hpp"
#include "rect.hpp"
#include "render_to_texture.hpp"

ImageComposer::ImageComposer(ImageSize image_size, const RectF& ortho_region_in,
                             GraphicsEngine& graphics_engine_in,
                             int msaa_samples_in)
    : width_(image_size.width),
      height_(image_size.height),
      ortho_region_(ortho_region_in),
      graphics_engine_(graphics_engine_in),
      bytes_per_pixel_(kBytesPerPixel),
      tile_size_(kMaxTileSize),
      msaa_samples_(msaa_samples_in) {
  CalculatePixelRemainders();
  CalculateTileOrthoSize();
  CalculateRemainderOrthoSize();
  CalculateTileGrid();

  ConfigureTiles();
  RenderTiles();
  if (!rendered_) {
    return;
  }
  ComposeTiles();
  VerticalFlip();
}

bool ImageComposer::Rendered() const { return rendered_; }

ImageTile& ImageComposer::TileAt(size_t column, size_t row) {
  return tiles_[column + (row * tile_columns_)];
}

std::vector<unsigned char> ImageComposer::CopyImage() const { return image_; }

size_t ImageComposer::TileCount() const { return tiles_.size(); }

size_t ImageComposer::TileColumns() const { return tile_columns_; }

size_t ImageComposer::TileRows() const { return tile_rows_; }

void ImageComposer::CalculatePixelRemainders() {
  width_remainder_ = width_ % tile_size_;
  height_remainder_ = height_ % tile_size_;
}

void ImageComposer::CalculateTileOrthoSize() {
  constexpr float kOne = 1.0F;
  float width_ratio = kOne;
  float height_ratio = kOne;

  if (width_ > tile_size_) {
    width_ratio = static_cast<float>(tile_size_) / static_cast<float>(width_);
  }

  if (height_ > tile_size_) {
    height_ratio = static_cast<float>(tile_size_) / static_cast<float>(height_);
  }

  tile_ortho_width_ = ortho_region_.Width() * width_ratio;
  tile_ortho_height_ = ortho_region_.Height() * height_ratio;
}

void ImageComposer::CalculateRemainderOrthoSize() {
  const float width_remainder_ratio =
      static_cast<float>(width_remainder_) / static_cast<float>(width_);
  const float height_remainder_ratio =
      static_cast<float>(height_remainder_) / static_cast<float>(height_);

  remainder_ortho_width_ = ortho_region_.Width() * width_remainder_ratio;
  remainder_ortho_height_ = ortho_region_.Height() * height_remainder_ratio;
}

void ImageComposer::CalculateTileGrid() {
  const size_t full_columns = width_ / tile_size_;
  const size_t full_rows = height_ / tile_size_;

  tile_columns_ = full_columns + (width_remainder_ > 0 ? 1 : 0);
  tile_rows_ = full_rows + (height_remainder_ > 0 ? 1 : 0);

  tiles_.resize(tile_columns_ * tile_rows_);
}

void ImageComposer::ConfigureTiles() {
  const bool width_has_remainder = width_remainder_ > 0;
  const bool height_has_remainder = height_remainder_ > 0;

  for (size_t row = 0; row < tile_rows_; ++row) {
    for (size_t column = 0; column < tile_columns_; ++column) {
      GLsizei tile_pixel_width = 0;
      GLsizei tile_pixel_height = 0;

      const auto column_float = static_cast<float>(column);
      const auto row_float = static_cast<float>(row);

      RectF tile_region;

      if ((column == tile_columns_ - 1) && width_has_remainder) {
        tile_pixel_width = static_cast<GLsizei>(width_remainder_);
        tile_region.SetLeft(ortho_region_.Left() +
                            (column_float * tile_ortho_width_));
        tile_region.SetRight(tile_region.Left() + remainder_ortho_width_);
      } else {
        tile_pixel_width = static_cast<GLsizei>(tile_size_);
        tile_region.SetLeft(ortho_region_.Left() +
                            (column_float * tile_ortho_width_));
        tile_region.SetRight(tile_region.Left() + tile_ortho_width_);
      }

      if ((row == tile_rows_ - 1) && height_has_remainder) {
        tile_pixel_height = static_cast<GLsizei>(height_remainder_);
        tile_region.SetBottom(ortho_region_.Bottom() +
                              (row_float * tile_ortho_height_));
        tile_region.SetTop(tile_region.Bottom() + remainder_ortho_height_);
      } else {
        tile_pixel_height = static_cast<GLsizei>(tile_size_);
        tile_region.SetBottom(ortho_region_.Bottom() +
                              (row_float * tile_ortho_height_));
        tile_region.SetTop(tile_region.Bottom() + tile_ortho_height_);
      }

      TileAt(column, row).pixel_dimensions[0] = tile_pixel_width;
      TileAt(column, row).pixel_dimensions[1] = tile_pixel_height;
      TileAt(column, row).ortho_region = tile_region;
    }
  }
}

void ImageComposer::RenderTiles() {
  MVP mvp;
  mvp.SetView(glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F)));

  for (size_t row = 0; row < TileRows(); ++row) {
    for (size_t column = 0; column < TileColumns(); ++column) {
      auto& tile = TileAt(column, row);

      RenderToTexture render_texture(tile.pixel_dimensions[0],
                                     tile.pixel_dimensions[1], msaa_samples_);
      // Without a complete framebuffer the render goes nowhere and the
      // read-back hands back a tile of zeros — a black page, silently. The
      // caller has to hear about it instead of writing the file.
      if (!render_texture.Valid()) {
        rendered_ = false;
        return;
      }
      if (decade_debug::LogEnabled() && row == 0 && column == 0) {
        std::cout << "--dump-png: " << render_texture.Samples()
                  << " samples per pixel (asked for " << msaa_samples_ << ")\n";
      }

      render_texture.BeginRender();
      mvp.SetProjection(
          glm::ortho(tile.ortho_region.Left(), tile.ortho_region.Right(),
                     tile.ortho_region.Bottom(), tile.ortho_region.Top()));

      graphics_engine_.SetMVP(mvp);
      graphics_engine_.Render();

      render_texture.EndRender();

      tile.pixels = render_texture.CopyImage();
    }
  }
}

void ImageComposer::ComposeTiles() {
  image_.resize(bytes_per_pixel_ * width_ * height_);

  size_t dest_x = 0;
  size_t dest_y = 0;

  for (size_t row = 0; row < TileRows(); ++row) {
    for (size_t column = 0; column < TileColumns(); ++column) {
      auto& tile = TileAt(column, row);

      const size_t dest_offset =
          (dest_y * width_ * bytes_per_pixel_) + (dest_x * bytes_per_pixel_);

      const auto tile_pixel_height =
          static_cast<size_t>(tile.pixel_dimensions[1]);
      for (size_t tile_row = 0; tile_row < tile_pixel_height; ++tile_row) {
        const size_t tile_row_bytes =
            static_cast<size_t>(tile.pixel_dimensions[0]) * bytes_per_pixel_;
        const size_t src_begin = tile_row * tile_row_bytes;
        const size_t src_end = src_begin + tile_row_bytes;

        const auto dest_row_offset = static_cast<std::ptrdiff_t>(
            dest_offset + (tile_row * width_ * bytes_per_pixel_));
        const auto src_begin_offset = static_cast<std::ptrdiff_t>(src_begin);
        const auto src_end_offset = static_cast<std::ptrdiff_t>(src_end);

        std::copy(tile.pixels.cbegin() + src_begin_offset,
                  tile.pixels.cbegin() + src_end_offset,
                  image_.begin() + dest_row_offset);
      }

      dest_x += static_cast<std::size_t>(tile.pixel_dimensions[0]);
    }

    auto& first_tile_in_row = TileAt(0, row);
    dest_x = 0;
    dest_y += static_cast<std::size_t>(first_tile_in_row.pixel_dimensions[1]);
  }
}

void ImageComposer::VerticalFlip() {
  const std::vector<unsigned char> image_copy(image_);

  for (size_t row = 0; row < height_; ++row) {
    const size_t row_size = width_ * bytes_per_pixel_;
    const size_t row_begin = row * row_size;
    const size_t row_end = row_begin + row_size;

    const auto row_begin_offset = static_cast<std::ptrdiff_t>(row_begin);
    const auto row_end_offset = static_cast<std::ptrdiff_t>(row_end);
    std::copy(image_copy.cbegin() + row_begin_offset,
              image_copy.cbegin() + row_end_offset,
              image_.end() - row_end_offset);
  }
}

namespace render_to_png_detail {

float DotsPerInchToDotsPerMillimeter(float dpi) {
  constexpr float kInchInMillimeters = 25.4F;
  return dpi / kInchInMillimeters;
}

bool FitsSizeT(float value) {
  const float limit =
      std::ldexp(1.0F, std::numeric_limits<std::size_t>::digits);
  return value >= 0.0F && value < limit;
}

}  // namespace render_to_png_detail

void WritePageToPng(const std::string& file_path, const RectF& ortho_region,
                    float dpi, GraphicsEngine& graphics_engine,
                    int msaa_samples) {
  const float dots_per_millimeter =
      render_to_png_detail::DotsPerInchToDotsPerMillimeter(dpi);
  const float width_pixels =
      std::round(ortho_region.Width() * dots_per_millimeter);
  const float height_pixels =
      std::round(ortho_region.Height() * dots_per_millimeter);
  if (!render_to_png_detail::FitsSizeT(width_pixels) ||
      !render_to_png_detail::FitsSizeT(height_pixels)) {
    return;
  }

  const auto image_width = static_cast<size_t>(width_pixels);
  const auto image_height = static_cast<size_t>(height_pixels);

  if (!png_io::FitsPngLimits(image_width, image_height)) {
    return;
  }

  const ImageComposer composer(
      ImageSize{.width = image_width, .height = image_height}, ortho_region,
      graphics_engine, msaa_samples);
  if (!composer.Rendered()) {
    std::cerr << "--dump-png: the off-screen framebuffer would not come up; "
                 "nothing written\n";
    return;
  }
  auto pixels = composer.CopyImage();
  png_io::WriteRgbaPng(
      file_path.c_str(), pixels,
      png_io::PngImageSize{.width = image_width, .height = image_height});
}
