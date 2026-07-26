#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <optional>
#include <string>
#include <vector>

#include "../domain/calendar_config.hpp"
#include "../domain/date_entry.hpp"
#include "../domain/date_group.hpp"
#include "../domain/page_setup_config.hpp"
#include "../domain/scene_snapshot.hpp"
#include "../domain/shape_configuration.hpp"
#include "../domain/state_topic.hpp"
#include "../domain/title_config.hpp"
#include "../infrastructure/graphics/pick_id.hpp"

// Zentraler typisierter Ereignisbus.
//
// Ein Topic je Domänenereignis, erreichbar über einen gleichnamigen Accessor.
// Produzenten veröffentlichen mit `bus.<topic>()(wert)`, Konsumenten hängen
// sich mit `bus.<topic>().connect(...)` an. Stores bekommen ihr Topic beim
// Bauen eingesetzt und rufen es selbst auf — dazwischen steht keine
// Weiterleitung mehr. Wer welchen Konsumenten anhängt, steht gesammelt in
// `main_window_binder`, damit keine Seite die andere kennen muss.
//
// Die Topics sind privat; die Accessors sind der einzige Zugang für beide
// Seiten.
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
  [[nodiscard]] auto& font_filepath() { return font_filepath_; }
  [[nodiscard]] auto& title_config() { return title_config_; }
  [[nodiscard]] auto& shape_config_set() { return shape_config_set_; }
  [[nodiscard]] auto& calendar_config() { return calendar_config_; }
  [[nodiscard]] auto& scene_snapshot() { return scene_snapshot_; }
  [[nodiscard]] auto& hovered() { return hovered_; }
  [[nodiscard]] auto& selected_node() { return selected_node_; }

 private:
  domain::StateTopic<std::vector<DateEntry>> date_entries_;
  domain::StateTopic<std::vector<DateEntry>> transformed_date_entries_;
  domain::StateTopic<std::vector<DateGroup>> date_groups_;
  domain::StateTopic<PageSetupConfig> page_setup_;
  domain::StateTopic<std::string> project_file_path_;
  domain::StateTopic<std::string> font_filepath_;
  domain::StateTopic<TitleConfig> title_config_;
  domain::StateTopic<ShapeConfigSet> shape_config_set_;
  domain::StateTopic<CalendarConfig> calendar_config_;
  domain::StateTopic<SceneNodeSnapshot> scene_snapshot_;
  domain::StateTopic<std::optional<PickId>> hovered_;
  domain::StateTopic<std::optional<std::string>> selected_node_;
};

#endif  // EVENT_BUS_HPP
