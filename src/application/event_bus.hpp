#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include "../domain/state_topics.hpp"
#include "interaction_topics.hpp"

// The central typed event bus.
//
// One topic per domain event, reachable through an accessor of the same name.
// Producers publish with `bus.<topic>().Publish(value)`, consumers attach with
// `QObject::connect(&bus.<topic>(), &Topic::Published, …)`. Stores get their
// topic injected on construction and publish themselves — no forwarding stands
// between any more. Who attaches which consumer sits collected in `app_binder`,
// so neither side needs to know the other.
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

  [[nodiscard]] domain::DateEntriesTopic& date_entries();
  [[nodiscard]] domain::DateEntriesTopic& transformed_date_entries();
  [[nodiscard]] domain::DateGroupsTopic& date_groups();
  [[nodiscard]] domain::PageSetupTopic& page_setup();
  [[nodiscard]] domain::FilePathTopic& project_file_path();
  [[nodiscard]] domain::FontConfigTopic& font_config();
  [[nodiscard]] domain::TitleConfigTopic& title_config();
  [[nodiscard]] domain::ShapeConfigSetTopic& shape_config_set();
  [[nodiscard]] domain::CalendarConfigTopic& calendar_config();
  [[nodiscard]] domain::SceneSnapshotTopic& scene_snapshot();
  [[nodiscard]] application::HoveredTopic& hovered();
  [[nodiscard]] domain::NodePathTopic& selected_node();
  [[nodiscard]] application::EditRequestTopic& edit_requested();
  [[nodiscard]] domain::TextEditTopic& text_edit();
  [[nodiscard]] domain::StateBurstTopic& state_burst();

 private:
  domain::DateEntriesTopic date_entries_;
  domain::DateEntriesTopic transformed_date_entries_;
  domain::DateGroupsTopic date_groups_;
  domain::PageSetupTopic page_setup_;
  domain::FilePathTopic project_file_path_;
  domain::FontConfigTopic font_config_;
  domain::TitleConfigTopic title_config_;
  domain::ShapeConfigSetTopic shape_config_set_;
  domain::CalendarConfigTopic calendar_config_;
  domain::SceneSnapshotTopic scene_snapshot_;
  application::HoveredTopic hovered_;
  domain::NodePathTopic selected_node_;
  // A double click on an element: please edit this one.
  application::EditRequestTopic edit_requested_;
  // What is to be seen of a running edit; empty means none.
  domain::TextEditTopic text_edit_;
  // The one topic that carries no state but a bracket around it: true opens a
  // burst of changes, false closes it. Loading a project fills six stores one
  // after another, and every one of them publishes — a consumer that rebuilds
  // on each would do the work six times for one user action (#36). It sits
  // here rather than behind a port of its own, because the producer
  // (ProjectDocument) exists long before the consumer (the rendering adapter,
  // which needs the GL context): over the bus, whoever is not there simply
  // does not listen.
  domain::StateBurstTopic state_burst_;
};

#endif  // EVENT_BUS_HPP
