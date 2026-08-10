#ifndef PAGE_GEOMETRY_HPP
#define PAGE_GEOMETRY_HPP

#include "../../domain/page_setup_config.hpp"
#include "rect.hpp"

// The one place that turns the page configuration into geometry. The same
// conversion used to stand in the GL canvas and in CalendarPage; the same
// knowledge in two places would have drifted apart eventually.

[[nodiscard]] RectF PageRect(const PageSetupConfig& page_setup_config);

[[nodiscard]] RectF PageMarginRect(const PageSetupConfig& page_setup_config);

#endif  // PAGE_GEOMETRY_HPP
