#ifndef RENDER_TO_PNG_HPP
#define RENDER_TO_PNG_HPP

#include <epoxy/gl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
                const std::shared_ptr<GraphicsEngine>& graphics_engine_in,
                int msaa_samples_in)
      : width(image_size.width),
        height(image_size.height),
        ortho_region(ortho_region_in),
        graphics_engine(graphics_engine_in),
        bytes_per_pixel(kBytesPerPixel),
        tile_size(kMaxTileSize),
        msaa_samples(msaa_samples_in) {
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
    return tiles[column + (row * tile_columns)];
  }

  [[nodiscard]] std::vector<unsigned char> CopyImage() const { return image; }

  [[nodiscard]] size_t TileCount() const { return tiles.size(); }

  [[nodiscard]] size_t TileColumns() const { return tile_columns; }

  [[nodiscard]] size_t TileRows() const { return tile_rows; }

 private:
  void CalculatePixelRemainders() {
    width_remainder = width % tile_size;
    height_remainder = height % tile_size;
  }

  void CalculateTileOrthoSize() {
    constexpr float kOne = 1.0F;
    float width_ratio = kOne;
    float height_ratio = kOne;

    if (width > tile_size) {
      width_ratio = static_cast<float>(tile_size) / static_cast<float>(width);
    }

    if (height > tile_size) {
      height_ratio = static_cast<float>(tile_size) / static_cast<float>(height);
    }

    tile_ortho_width = ortho_region.width() * width_ratio;
    tile_ortho_height = ortho_region.height() * height_ratio;
  }

  void CalculateRemainderOrthoSize() {
    const float width_remainder_ratio =
        static_cast<float>(width_remainder) / static_cast<float>(width);
    const float height_remainder_ratio =
        static_cast<float>(height_remainder) / static_cast<float>(height);

    remainder_ortho_width = ortho_region.width() * width_remainder_ratio;
    remainder_ortho_height = ortho_region.height() * height_remainder_ratio;
  }

  void CalculateTileGrid() {
    const size_t full_columns = width / tile_size;
    const size_t full_rows = height / tile_size;

    tile_columns = full_columns + (width_remainder > 0 ? 1 : 0);
    tile_rows = full_rows + (height_remainder > 0 ? 1 : 0);

    tiles.resize(tile_columns * tile_rows);
  }

  void ConfigureTiles() {
    const bool width_has_remainder = width_remainder > 0;
    const bool height_has_remainder = height_remainder > 0;

    for (size_t row = 0; row < tile_rows; ++row) {
      for (size_t column = 0; column < tile_columns; ++column) {
        GLsizei tile_pixel_width = 0;
        GLsizei tile_pixel_height = 0;

        const auto column_float = static_cast<float>(column);
        const auto row_float = static_cast<float>(row);

        rectf tile_region;

        if ((column == tile_columns - 1) && width_has_remainder) {
          tile_pixel_width = static_cast<GLsizei>(width_remainder);
          tile_region.setL(ortho_region.l() +
                           (column_float * tile_ortho_width));
          tile_region.setR(tile_region.l() + remainder_ortho_width);
        } else {
          tile_pixel_width = static_cast<GLsizei>(tile_size);
          tile_region.setL(ortho_region.l() +
                           (column_float * tile_ortho_width));
          tile_region.setR(tile_region.l() + tile_ortho_width);
        }

        if ((row == tile_rows - 1) && height_has_remainder) {
          tile_pixel_height = static_cast<GLsizei>(height_remainder);
          tile_region.setB(ortho_region.b() + (row_float * tile_ortho_height));
          tile_region.setT(tile_region.b() + remainder_ortho_height);
        } else {
          tile_pixel_height = static_cast<GLsizei>(tile_size);
          tile_region.setB(ortho_region.b() + (row_float * tile_ortho_height));
          tile_region.setT(tile_region.b() + tile_ortho_height);
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
                                       tile.pixel_dimensions[1], msaa_samples);

        render_texture.BeginRender();
        mvp.SetProjection(
            glm::ortho(tile.ortho_region.l(), tile.ortho_region.r(),
                       tile.ortho_region.b(), tile.ortho_region.t()));

        graphics_engine->SetMVP(mvp);
        graphics_engine->Render();

        render_texture.EndRender();

        tile.pixels = render_texture.CopyImage();
      }
    }
  }

  void ComposeTiles() {
    image.resize(bytes_per_pixel * width * height);

    size_t dest_x = 0;
    size_t dest_y = 0;

    for (size_t row = 0; row < TileRows(); ++row) {
      for (size_t column = 0; column < TileColumns(); ++column) {
        auto& tile = TileAt(column, row);

        const size_t dest_offset =
            (dest_y * width * bytes_per_pixel) + (dest_x * bytes_per_pixel);

        const auto tile_pixel_height =
            static_cast<size_t>(tile.pixel_dimensions[1]);
        for (size_t tile_row = 0; tile_row < tile_pixel_height; ++tile_row) {
          const size_t tile_row_bytes =
              static_cast<size_t>(tile.pixel_dimensions[0]) * bytes_per_pixel;
          const size_t src_begin = tile_row * tile_row_bytes;
          const size_t src_end = src_begin + tile_row_bytes;

          const auto dest_row_offset = static_cast<std::ptrdiff_t>(
              dest_offset + (tile_row * width * bytes_per_pixel));
          const auto src_begin_offset = static_cast<std::ptrdiff_t>(src_begin);
          const auto src_end_offset = static_cast<std::ptrdiff_t>(src_end);

          std::copy(tile.pixels.cbegin() + src_begin_offset,
                    tile.pixels.cbegin() + src_end_offset,
                    image.begin() + dest_row_offset);
        }

        dest_x += static_cast<std::size_t>(tile.pixel_dimensions[0]);
      }

      auto& first_tile_in_row = TileAt(0, row);
      dest_x = 0;
      dest_y += static_cast<std::size_t>(first_tile_in_row.pixel_dimensions[1]);
    }
  }

  void VerticalFlip() {
    const std::vector<unsigned char> image_copy(image);

    for (size_t row = 0; row < height; ++row) {
      const size_t row_size = width * bytes_per_pixel;
      const size_t row_begin = row * row_size;
      const size_t row_end = row_begin + row_size;

      const auto row_begin_offset = static_cast<std::ptrdiff_t>(row_begin);
      const auto row_end_offset = static_cast<std::ptrdiff_t>(row_end);
      std::copy(image_copy.cbegin() + row_begin_offset,
                image_copy.cbegin() + row_end_offset,
                image.end() - row_end_offset);
    }
  }

  size_t width{0};
  size_t height{0};

  rectf ortho_region;

  std::vector<ImageTile> tiles;
  size_t tile_columns{0};
  size_t tile_rows{0};

  std::shared_ptr<GraphicsEngine> graphics_engine;

  size_t bytes_per_pixel{0};
  std::vector<unsigned char> image;

  size_t tile_size{0};
  size_t width_remainder{0};
  size_t height_remainder{0};
  float tile_ortho_width{0.0F};
  float tile_ortho_height{0.0F};
  float remainder_ortho_width{0.0F};
  float remainder_ortho_height{0.0F};
  int msaa_samples;

  static constexpr size_t kBytesPerPixel = 4;
  static constexpr size_t kMaxTileSize = 4096;
};

// Renders the calendar page to a PNG at a given resolution. Converts the page's
// orthographic millimetre extent into a pixel size via the requested dpi, then
// delegates tiled rendering to ImageComposer and file writing to png_io.
class RenderToPNG {
 public:
  RenderToPNG(std::string file_path_in, const rectf& ortho_region_in,
              const float dpi_in,
              std::shared_ptr<GraphicsEngine> graphics_engine_in,
              int msaa_samples_in)
      : file_path(std::move(file_path_in)),
        ortho_region(ortho_region_in),
        dpi(dpi_in),
        graphics_engine(std::move(graphics_engine_in)),
        msaa_samples(msaa_samples_in) {
    RenderPicture();
  }

  void RenderPicture() {
    // calculate the rounded pixel dimension from dpi
    const float dots_per_millimeter = DotsPerInchToDotsPerMillimeter(dpi);
    const float pixel_width =
        std::round(ortho_region.width() * dots_per_millimeter);
    const float pixel_height =
        std::round(ortho_region.height() * dots_per_millimeter);

    image_width = static_cast<size_t>(std::max(0.0F, pixel_width));
    image_height = static_cast<size_t>(std::max(0.0F, pixel_height));

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

 private:
  [[nodiscard]] static float DotsPerInchToDotsPerMillimeter(
      const float dpi_value) {
    constexpr float ratio = 1.0F / 25.4F;
    return ratio * dpi_value;
  }

  std::string file_path;
  rectf ortho_region;
  float dpi;
  size_t image_width{0};
  size_t image_height{0};
  std::shared_ptr<GraphicsEngine> graphics_engine;
  int msaa_samples;
};
#endif  // RENDER_TO_PNG_HPP
