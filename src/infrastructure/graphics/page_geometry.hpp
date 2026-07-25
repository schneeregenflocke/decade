#ifndef PAGE_GEOMETRY_HPP
#define PAGE_GEOMETRY_HPP

#include "../../domain/page_setup_config.hpp"
#include "rect.hpp"

// Die eine Stelle, die aus der Seitenkonfiguration Geometrie macht. Vorher
// stand dieselbe Umrechnung im GL-Canvas und in der CalendarPage; dasselbe
// Wissen an zwei Orten wäre irgendwann auseinandergelaufen.

[[nodiscard]] inline rectf PageRect(const PageSetupConfig& page_setup_config) {
  return rectf::from_dimension(
      rectf::Dimension{.width = page_setup_config.Size()[0],
                       .height = page_setup_config.Size()[1]});
}

[[nodiscard]] inline rectf PageMarginRect(
    const PageSetupConfig& page_setup_config) {
  return {page_setup_config.Margins()[0], page_setup_config.Margins()[1],
          page_setup_config.Margins()[2], page_setup_config.Margins()[3]};
}

#endif  // PAGE_GEOMETRY_HPP
