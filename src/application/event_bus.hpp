#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include "../domain/state_topics.hpp"
#include "interaction_topics.hpp"

// The central typed event bus, one topic per domain event. Producers publish
// with `bus.<topic>.Publish(value)`, consumers attach with
// `QObject::connect(&bus.<topic>, &Topic::Published, …)` — collected in
// `app_binder`, so neither side needs to know the other. What travels which
// way: AGENTS.md, "Event flow".
//
// An aggregate rather than a class with accessors: those handed out non-const
// references and thereby guarded nothing. No member function may come back
// either — `misc-non-private-member-variables-in-classes` fires the moment one
// record carries data and user-declared functions at once. Nothing is lost by
// it: QObject members are not copyable, so neither is the bus.
struct EventBus {
  domain::DateEntriesTopic date_entries;
  domain::DateEntriesTopic transformed_date_entries;
  domain::DateGroupsTopic date_groups;
  domain::PageSetupTopic page_setup;
  domain::FilePathTopic project_file_path;
  domain::FontConfigTopic font_config;
  domain::TitleConfigTopic title_config;
  domain::ShapeConfigSetTopic shape_config_set;
  domain::CalendarConfigTopic calendar_config;
  domain::SceneSnapshotTopic scene_snapshot;
  application::HoveredTopic hovered;
  domain::NodePathTopic selected_node;
  // A double click on an element: please edit this one.
  application::EditRequestTopic edit_requested;
  // What is to be seen of a running edit; empty means none.
  domain::TextEditTopic text_edit;
  domain::StateBurstTopic state_burst;
};

#endif  // EVENT_BUS_HPP
