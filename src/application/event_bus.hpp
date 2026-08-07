#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <optional>
#include <string>
#include <vector>

#include "../domain/calendar_config.hpp"
#include "../domain/date_entry.hpp"
#include "../domain/date_group.hpp"
#include "../domain/font_config.hpp"
#include "../domain/page_setup_config.hpp"
#include "../domain/scene_snapshot.hpp"
#include "../domain/shape_configuration.hpp"
#include "../domain/state_topic.hpp"
#include "../domain/text_edit_view.hpp"
#include "../domain/title_config.hpp"
#include "../infrastructure/graphics/pick_id.hpp"

// The central typed event bus.
//
// One topic per domain event, reachable through an accessor of the same name.
// Producers publish with `bus.<topic>()(value)`, consumers attach with
// `bus.<topic>().connect(...)`. Stores get their topic injected on construction
// and call it themselves — no forwarding stands between any more. Who attaches
// which consumer sits collected in `main_window_binder`, so neither side needs
// to know the other.
//
// The topics are private; the accessors are the only way in for both sides.
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
  [[nodiscard]] auto& project_file_path() { return project_file_path_; }
  [[nodiscard]] auto& font_config() { return font_config_; }
  [[nodiscard]] auto& title_config() { return title_config_; }
  [[nodiscard]] auto& shape_config_set() { return shape_config_set_; }
  [[nodiscard]] auto& calendar_config() { return calendar_config_; }
  [[nodiscard]] auto& scene_snapshot() { return scene_snapshot_; }
  [[nodiscard]] auto& hovered() { return hovered_; }
  [[nodiscard]] auto& selected_node() { return selected_node_; }
  [[nodiscard]] auto& edit_requested() { return edit_requested_; }
  [[nodiscard]] auto& text_edit() { return text_edit_; }
  [[nodiscard]] auto& state_burst() { return state_burst_; }

 private:
  domain::StateTopic<std::vector<DateEntry>> date_entries_;
  domain::StateTopic<std::vector<DateEntry>> transformed_date_entries_;
  domain::StateTopic<std::vector<DateGroup>> date_groups_;
  domain::StateTopic<PageSetupConfig> page_setup_;
  domain::StateTopic<std::string> project_file_path_;
  domain::StateTopic<FontConfig> font_config_;
  domain::StateTopic<TitleConfig> title_config_;
  domain::StateTopic<ShapeConfigSet> shape_config_set_;
  domain::StateTopic<CalendarConfig> calendar_config_;
  domain::StateTopic<SceneNodeSnapshot> scene_snapshot_;
  domain::StateTopic<std::optional<PickId>> hovered_;
  domain::StateTopic<std::optional<std::string>> selected_node_;
  // A double click on an element: please edit this one.
  domain::StateTopic<PickId> edit_requested_;
  // What is to be seen of a running edit; empty means none.
  domain::StateTopic<std::optional<TextEditView>> text_edit_;
  // The one topic that carries no state but a bracket around it: true opens a
  // burst of changes, false closes it. Loading a project fills six stores one
  // after another, and every one of them publishes — a consumer that rebuilds
  // on each would do the work six times for one user action (#36). It sits
  // here rather than behind a port of its own, because the producer
  // (ProjectDocument) exists long before the consumer (the rendering adapter,
  // which needs the GL context): over the bus, whoever is not there simply
  // does not listen.
  domain::StateTopic<bool> state_burst_;
};

#endif  // EVENT_BUS_HPP
