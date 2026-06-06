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
// One sigslot signal per domain event. Producers (panels, stores, file I/O)
// emit by calling `bus.<event>(value)`; consumers attach via
// `bus.<event>.connect(...)`. The `main_window_binder` namespace wires
// concrete components to these signals so neither side has to know about the
// other.
class EventBus {
 public:
  EventBus() = default;
  ~EventBus() = default;
  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;
  EventBus(EventBus&&) = delete;
  EventBus& operator=(EventBus&&) = delete;

  sigslot::signal<const std::vector<DateIntervalBundle>&> date_interval_bundles;
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      transformed_date_interval_bundles;
  sigslot::signal<const std::vector<DateGroup>&> date_groups;
  sigslot::signal<const PageSetupConfig&> page_setup;
  sigslot::signal<const std::string&> font_filepath;
  sigslot::signal<const TitleConfig&> title_config;
  sigslot::signal<const ShapeConfigurationStorage&> shape_configuration_storage;
  sigslot::signal<const CalendarConfigStorage&> calendar_config_storage;
};

#endif  // EVENT_BUS_HPP
