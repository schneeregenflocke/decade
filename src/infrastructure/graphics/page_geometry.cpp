#include "page_geometry.hpp"

#include <array>

#include "../../domain/page_setup_config.hpp"
#include "rect.hpp"

RectF PageRect(const PageSetupConfig& page_setup_config) {
  return RectF::FromDimension(
      RectF::Dimension{.width = page_setup_config.Size()[0],
                       .height = page_setup_config.Size()[1]});
}

RectF PageMarginRect(const PageSetupConfig& page_setup_config) {
  const std::array<float, 4>& margins = page_setup_config.Margins();
  return {margins[0], margins[2], margins[1], margins[3]};
}
