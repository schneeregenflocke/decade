#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include "../domain/state_topics.hpp"
#include "interaction_topics.hpp"

// The central typed event bus.
//
// One topic per domain event. Producers publish with
// `bus.<topic>.Publish(value)`, consumers attach with
// `QObject::connect(&bus.<topic>, &Topic::Published, …)`. Stores get their
// topic injected on construction and publish themselves — no forwarding stands
// between any more. Who attaches which consumer sits collected in `app_binder`,
// so neither side needs to know the other.
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
  // The one topic that carries no state but a bracket around it: true opens a
  // burst of changes, false closes it. Loading a project fills six stores one
  // after another, and every one of them publishes — a consumer that rebuilds
  // on each would do the work six times for one user action (#36). It sits
  // here rather than behind a port of its own, because the producer
  // (ProjectDocument) exists long before the consumer (the rendering adapter,
  // which needs the GL context): over the bus, whoever is not there simply
  // does not listen.
  domain::StateBurstTopic state_burst;
};

#endif  // EVENT_BUS_HPP
