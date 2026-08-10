#include "event_bus.hpp"

#include "../domain/state_topics.hpp"
#include "interaction_topics.hpp"

domain::DateEntriesTopic& EventBus::date_entries() { return date_entries_; }

domain::DateEntriesTopic& EventBus::transformed_date_entries() {
  return transformed_date_entries_;
}

domain::DateGroupsTopic& EventBus::date_groups() { return date_groups_; }

domain::PageSetupTopic& EventBus::page_setup() { return page_setup_; }

domain::FilePathTopic& EventBus::project_file_path() {
  return project_file_path_;
}

domain::FontConfigTopic& EventBus::font_config() { return font_config_; }

domain::TitleConfigTopic& EventBus::title_config() { return title_config_; }

domain::ShapeConfigSetTopic& EventBus::shape_config_set() {
  return shape_config_set_;
}

domain::CalendarConfigTopic& EventBus::calendar_config() {
  return calendar_config_;
}

domain::SceneSnapshotTopic& EventBus::scene_snapshot() {
  return scene_snapshot_;
}

application::HoveredTopic& EventBus::hovered() { return hovered_; }

domain::NodePathTopic& EventBus::selected_node() { return selected_node_; }

application::EditRequestTopic& EventBus::edit_requested() {
  return edit_requested_;
}

domain::TextEditTopic& EventBus::text_edit() { return text_edit_; }

domain::StateBurstTopic& EventBus::state_burst() { return state_burst_; }
