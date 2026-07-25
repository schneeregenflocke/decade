#ifndef RENDER_TO_PNG_HPP
#define RENDER_TO_PNG_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <glm/gtx/transform.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "graphics_engine.hpp"
#include "mvp_matrices.hpp"
#include "png_writer.hpp"
#include "render_to_texture.hpp"

// One tile of the final image. The full picture is rendered in pieces because
// a single FBO/texture is capped at kMaxTileSize (4096) per side; a 200-dpi
// page easily exceeds that. `pixel_dimensions` is the tile's {width, height} in
// pixels, `ortho_region` the slice of the orthographic world it maps to,
// `pixels` the rendered RGBA bytes.
//
// On MSAA at the seams (a question worth recording): the tiles do *not* overlap
// and they must not. Each tile covers an integer-pixel region whose projection
// maps that region exactly onto the tile's pixel grid, and every tile boundary
// falls on a pixel edge that both neighbours share. So every output pixel is
// owned by exactly one tile and is MSAA-resolved (the blit in RenderToTexture)
// using only that tile's samples — there is no pixel split across two resolves.
// The full scene is drawn into every tile, just clipped by the per-tile
// projection, so geometry crossing a seam gets correct coverage on each side
// and the halves line up. Adding overlap would re-render shared pixels and
// blend them twice, which is exactly what we avoid.
struct ImageTile {
  std::array<GLsizei, 2> pixel_dimensions{0, 0};  // {width, height} in pixels
  rectf ortho_region;                             // world-space slice it maps
  std::vector<unsigned char> pixels;              // rendered RGBA bytes
};

struct ImageSize {
  size_t width{0};
  size_t height{0};
};

// Renders a large orthographic scene into a grid of tiles and composes them
// into a single top-left-origin RGBA image. The grid has `tile_columns` x
// `tile_rows` tiles; every tile is kMaxTileSize wide/high except the last
// column/row, which hold the leftover ("remainder") pixels.
class ImageComposer {
 public:
  ImageComposer(ImageSize image_size, const rectf& ortho_region_in,
                GraphicsEngine& graphics_engine_in, int msaa_samples_in)
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
    ComposeTiles();
    VerticalFlip();
  }

  ImageTile& TileAt(size_t column, size_t row) {
    return tiles_[column + (row * tile_columns_)];
  }

  [[nodiscard]] std::vector<unsigned char> CopyImage() const { return image_; }

  [[nodiscard]] size_t TileCount() const { return tiles_.size(); }

  [[nodiscard]] size_t TileColumns() const { return tile_columns_; }

  [[nodiscard]] size_t TileRows() const { return tile_rows_; }

 private:
  void CalculatePixelRemainders() {
    width_remainder_ = width_ % tile_size_;
    height_remainder_ = height_ % tile_size_;
  }

  void CalculateTileOrthoSize() {
    constexpr float kOne = 1.0F;
    float width_ratio = kOne;
    float height_ratio = kOne;

    if (width_ > tile_size_) {
      width_ratio = static_cast<float>(tile_size_) / static_cast<float>(width_);
    }

    if (height_ > tile_size_) {
      height_ratio =
          static_cast<float>(tile_size_) / static_cast<float>(height_);
    }

    tile_ortho_width_ = ortho_region_.width() * width_ratio;
    tile_ortho_height_ = ortho_region_.height() * height_ratio;
  }

  void CalculateRemainderOrthoSize() {
    const float width_remainder_ratio =
        static_cast<float>(width_remainder_) / static_cast<float>(width_);
    const float height_remainder_ratio =
        static_cast<float>(height_remainder_) / static_cast<float>(height_);

    remainder_ortho_width_ = ortho_region_.width() * width_remainder_ratio;
    remainder_ortho_height_ = ortho_region_.height() * height_remainder_ratio;
  }

  void CalculateTileGrid() {
    const size_t full_columns = width_ / tile_size_;
    const size_t full_rows = height_ / tile_size_;

    tile_columns_ = full_columns + (width_remainder_ > 0 ? 1 : 0);
    tile_rows_ = full_rows + (height_remainder_ > 0 ? 1 : 0);

    tiles_.resize(tile_columns_ * tile_rows_);
  }

  void ConfigureTiles() {
    const bool width_has_remainder = width_remainder_ > 0;
    const bool height_has_remainder = height_remainder_ > 0;

    for (size_t row = 0; row < tile_rows_; ++row) {
      for (size_t column = 0; column < tile_columns_; ++column) {
        GLsizei tile_pixel_width = 0;
        GLsizei tile_pixel_height = 0;

        const auto column_float = static_cast<float>(column);
        const auto row_float = static_cast<float>(row);

        rectf tile_region;

        if ((column == tile_columns_ - 1) && width_has_remainder) {
          tile_pixel_width = static_cast<GLsizei>(width_remainder_);
          tile_region.setL(ortho_region_.l() +
                           (column_float * tile_ortho_width_));
          tile_region.setR(tile_region.l() + remainder_ortho_width_);
        } else {
          tile_pixel_width = static_cast<GLsizei>(tile_size_);
          tile_region.setL(ortho_region_.l() +
                           (column_float * tile_ortho_width_));
          tile_region.setR(tile_region.l() + tile_ortho_width_);
        }

        if ((row == tile_rows_ - 1) && height_has_remainder) {
          tile_pixel_height = static_cast<GLsizei>(height_remainder_);
          tile_region.setB(ortho_region_.b() +
                           (row_float * tile_ortho_height_));
          tile_region.setT(tile_region.b() + remainder_ortho_height_);
        } else {
          tile_pixel_height = static_cast<GLsizei>(tile_size_);
          tile_region.setB(ortho_region_.b() +
                           (row_float * tile_ortho_height_));
          tile_region.setT(tile_region.b() + tile_ortho_height_);
        }

        TileAt(column, row).pixel_dimensions[0] = tile_pixel_width;
        TileAt(column, row).pixel_dimensions[1] = tile_pixel_height;
        TileAt(column, row).ortho_region = tile_region;
      }
    }
  }

  void RenderTiles() {
    MVP mvp;
    mvp.SetView(glm::translate(glm::vec3(0.0F, 0.0F, 0.0F)));

    for (size_t row = 0; row < TileRows(); ++row) {
      for (size_t column = 0; column < TileColumns(); ++column) {
        auto& tile = TileAt(column, row);

        RenderToTexture render_texture(tile.pixel_dimensions[0],
                                       tile.pixel_dimensions[1], msaa_samples_);

        render_texture.BeginRender();
        mvp.SetProjection(
            glm::ortho(tile.ortho_region.l(), tile.ortho_region.r(),
                       tile.ortho_region.b(), tile.ortho_region.t()));

        graphics_engine_.SetMVP(mvp);
        graphics_engine_.Render();

        render_texture.EndRender();

        tile.pixels = render_texture.CopyImage();
      }
    }
  }

  void ComposeTiles() {
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

  void VerticalFlip() {
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

  size_t width_{0};
  size_t height_{0};

  rectf ortho_region_;

  std::vector<ImageTile> tiles_;
  size_t tile_columns_{0};
  size_t tile_rows_{0};

  GraphicsEngine& graphics_engine_;

  size_t bytes_per_pixel_{0};
  std::vector<unsigned char> image_;

  size_t tile_size_{0};
  size_t width_remainder_{0};
  size_t height_remainder_{0};
  float tile_ortho_width_{0.0F};
  float tile_ortho_height_{0.0F};
  float remainder_ortho_width_{0.0F};
  float remainder_ortho_height_{0.0F};
  int msaa_samples_;

  static constexpr size_t kBytesPerPixel = 4;
  static constexpr size_t kMaxTileSize = 4096;
};

namespace render_to_png_detail {

[[nodiscard]] inline float DotsPerInchToDotsPerMillimeter(float dpi) {
  constexpr float kInchInMillimeters = 25.4F;
  return dpi / kInchInMillimeters;
}

}  // namespace render_to_png_detail

// Zeichnet die Kalenderseite in der gewünschten Auflösung als PNG. Rechnet die
// Millimeter-Ausdehnung der Seite über die dpi in eine Pixelgrösse um, lässt
// ImageComposer gekachelt rendern und png_io schreiben. Tut nichts, wenn die
// Bildgrösse die PNG-Grenzen sprengt.
inline void WritePageToPng(const std::string& file_path,
                           const rectf& ortho_region, float dpi,
                           GraphicsEngine& graphics_engine, int msaa_samples) {
  const float dots_per_millimeter =
      render_to_png_detail::DotsPerInchToDotsPerMillimeter(dpi);
  const auto image_width = static_cast<size_t>(
      std::max(0.0F, std::round(ortho_region.width() * dots_per_millimeter)));
  const auto image_height = static_cast<size_t>(
      std::max(0.0F, std::round(ortho_region.height() * dots_per_millimeter)));

  if (!png_io::FitsPngLimits(image_width, image_height)) {
    return;
  }

  const ImageComposer composer(
      ImageSize{.width = image_width, .height = image_height}, ortho_region,
      graphics_engine, msaa_samples);
  auto pixels = composer.CopyImage();
  png_io::WriteRgbaPng(
      file_path.c_str(), pixels,
      png_io::PngImageSize{.width = image_width, .height = image_height});
}
#endif  // RENDER_TO_PNG_HPP
