#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <optional>
#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "../../graphics/pick_id.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date_entry.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/page_setup_config.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/title_config.hpp"
#include "scene_snapshot.hpp"

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

  [[nodiscard]] auto& date_entries() { return date_entries_; }
  [[nodiscard]] auto& transformed_date_entries() {
    return transformed_date_entries_;
  }
  [[nodiscard]] auto& date_groups() { return date_groups_; }
  [[nodiscard]] auto& page_setup() { return page_setup_; }
  [[nodiscard]] auto& font_filepath() { return font_filepath_; }
  [[nodiscard]] auto& title_config() { return title_config_; }
  [[nodiscard]] auto& shape_config_set() { return shape_config_set_; }
  [[nodiscard]] auto& calendar_config() { return calendar_config_; }
  [[nodiscard]] auto& scene_snapshot() { return scene_snapshot_; }
  [[nodiscard]] auto& hovered() { return hovered_; }

 private:
  sigslot::signal<const std::vector<DateEntry>&> date_entries_;
  sigslot::signal<const std::vector<DateEntry>&> transformed_date_entries_;
  sigslot::signal<const std::vector<DateGroup>&> date_groups_;
  sigslot::signal<const PageSetupConfig&> page_setup_;
  sigslot::signal<const std::string&> font_filepath_;
  sigslot::signal<const TitleConfig&> title_config_;
  sigslot::signal<const ShapeConfigSet&> shape_config_set_;
  sigslot::signal<const CalendarConfig&> calendar_config_;
  sigslot::signal<const SceneNodeSnapshot&> scene_snapshot_;
  sigslot::signal<const std::optional<PickId>&> hovered_;
};

#endif  // EVENT_BUS_HPP
