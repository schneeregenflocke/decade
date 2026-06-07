#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../../packages/calendar_config.hpp"
#include "../../packages/date_store.hpp"
#include "../../packages/group_store.hpp"
#include "../../packages/page_config.hpp"
#include "../../packages/shape_config.hpp"
#include "../../packages/title_config.hpp"

// Central typed event bus for cross-component communication.
//
// One sigslot signal per domain event, reached through a same-named accessor.
// Producers (panels, stores, file I/O) emit by calling `bus.<event>()(value)`;
// consumers attach via `bus.<event>().connect(...)`. The signals themselves are
// private — the accessors are the single point through which both sides reach
// them. The `main_window_binder` namespace wires concrete components so neither
// side has to know about the other.
class EventBus {
 public:
  EventBus() = default;
  ~EventBus() = default;
  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;
  EventBus(EventBus&&) = delete;
  EventBus& operator=(EventBus&&) = delete;

  [[nodiscard]] auto& date_interval_bundles() { return date_interval_bundles_; }
  [[nodiscard]] auto& transformed_date_interval_bundles() {
    return transformed_date_interval_bundles_;
  }
  [[nodiscard]] auto& date_groups() { return date_groups_; }
  [[nodiscard]] auto& page_setup() { return page_setup_; }
  [[nodiscard]] auto& font_filepath() { return font_filepath_; }
  [[nodiscard]] auto& title_config() { return title_config_; }
  [[nodiscard]] auto& shape_config_set() { return shape_config_set_; }
  [[nodiscard]] auto& calendar_config() { return calendar_config_; }

 private:
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      date_interval_bundles_;
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      transformed_date_interval_bundles_;
  sigslot::signal<const std::vector<DateGroup>&> date_groups_;
  sigslot::signal<const PageSetupConfig&> page_setup_;
  sigslot::signal<const std::string&> font_filepath_;
  sigslot::signal<const TitleConfig&> title_config_;
  sigslot::signal<const ShapeConfigSet&> shape_config_set_;
  sigslot::signal<const CalendarConfig&> calendar_config_;
};

#endif  // EVENT_BUS_HPP
