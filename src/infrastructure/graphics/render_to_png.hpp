#ifndef RENDER_TO_PNG_HPP
#define RENDER_TO_PNG_HPP

#include <epoxy/gl.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "graphics_engine.hpp"
#include "rect.hpp"

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
  RectF ortho_region;                             // world-space slice it maps
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
  ImageComposer(ImageSize image_size, const RectF& ortho_region_in,
                GraphicsEngine& graphics_engine_in, int msaa_samples_in);

  // False when a tile's framebuffer would not come up. The composed image is
  // then empty, and nothing may get written out of it.
  [[nodiscard]] bool Rendered() const;

  ImageTile& TileAt(size_t column, size_t row);

  [[nodiscard]] std::vector<unsigned char> CopyImage() const;

  [[nodiscard]] size_t TileCount() const;

  [[nodiscard]] size_t TileColumns() const;

  [[nodiscard]] size_t TileRows() const;

 private:
  void CalculatePixelRemainders();

  void CalculateTileOrthoSize();

  void CalculateRemainderOrthoSize();

  void CalculateTileGrid();

  void ConfigureTiles();

  void RenderTiles();

  void ComposeTiles();

  void VerticalFlip();

  size_t width_{0};
  size_t height_{0};

  RectF ortho_region_;

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
  bool rendered_{true};

  static constexpr size_t kBytesPerPixel = 4;
  static constexpr size_t kMaxTileSize = 4096;
};

namespace render_to_png_detail {

[[nodiscard]] float DotsPerInchToDotsPerMillimeter(float dpi);

// Whether `value` survives a static_cast to size_t. That cast is undefined for
// anything outside the target's range, so the range gets tested while the value
// is still a float — FitsPngLimits sees the number after the cast and therefore
// too late. The page size comes out of a project file unvalidated
// (`value_serialization.hpp`, no clamp in `SetSize`), so an absurd extent times
// the dpi does reach here.
//
// 2^64 is the first value a 64-bit size_t cannot hold; NaN fails the lower
// test, which is the point of writing it as `>= 0` rather than `!(< 0)`.
[[nodiscard]] bool FitsSizeT(float value);

}  // namespace render_to_png_detail

// Draws the calendar page as a PNG at the wanted resolution. It converts the
// page's millimetre extent into a pixel size through the dpi, has ImageComposer
// render it in tiles and png_io write it. It does nothing when the image size
// bursts the PNG limits.
void WritePageToPng(const std::string& file_path, const RectF& ortho_region,
                    float dpi, GraphicsEngine& graphics_engine,
                    int msaa_samples);
#endif  // RENDER_TO_PNG_HPP
