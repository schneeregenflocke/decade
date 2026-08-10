#include "page_geometry.hpp"

#include "../../domain/page_setup_config.hpp"
#include "rect.hpp"

RectF PageRect(const PageSetupConfig& page_setup_config) {
  return RectF::FromDimension(
      RectF::Dimension{.width = page_setup_config.Size()[0],
                       .height = page_setup_config.Size()[1]});
}

RectF PageMarginRect(const PageSetupConfig& page_setup_config) {
  return {page_setup_config.Margins()[0], page_setup_config.Margins()[1],
          page_setup_config.Margins()[2], page_setup_config.Margins()[3]};
}
