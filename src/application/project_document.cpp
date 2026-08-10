#include "project_document.hpp"

#include <optional>
#include <string>
#include <utility>

#include "../domain/calendar_config_store.hpp"
#include "../domain/date_entry_store.hpp"
#include "../domain/date_format.hpp"
#include "../domain/date_group_store.hpp"
#include "../domain/page_setup_store.hpp"
#include "../domain/shape_configuration_store.hpp"
#include "../domain/title_config_store.hpp"
#include "../domain/transform_date_entry.hpp"
#include "../infrastructure/persistence/csv_io.hpp"
#include "../infrastructure/persistence/project_io.hpp"
#include "event_bus.hpp"
#include "state_burst.hpp"

namespace application {

ProjectDocument::ProjectDocument(EventBus& bus,
                                 LocaleDateFormatter& locale_date_formatter)
    : locale_date_formatter_(locale_date_formatter),
      file_path_topic_(bus.project_file_path()),
      state_burst_topic_(bus.state_burst()),
      date_groups_store_(bus.date_groups()),
      date_entry_store_(bus.date_entries()),
      transform_date_entry_(bus.transformed_date_entries()),
      page_setup_store_(bus.page_setup()),
      title_config_store_(bus.title_config()),
      shape_configuration_store_(bus.shape_config_set()),
      calendar_configuration_store_(bus.calendar_config()) {}

std::optional<std::string> ProjectDocument::LoadXml(std::string file_path) {
  // Six stores get filled one after another and every one of them publishes.
  // The bracket makes that one change for whoever rebuilds on it (#36).
  const StateBurst burst(state_burst_topic_);
  if (auto error = persistence::LoadProjectXml(
          file_path, date_groups_store_, date_entry_store_, page_setup_store_,
          title_config_store_, shape_configuration_store_,
          calendar_configuration_store_)) {
    return error;
  }
  SetFilePath(std::move(file_path));
  return std::nullopt;
}

std::optional<std::string> ProjectDocument::SaveXml(std::string file_path) {
  if (auto error = persistence::SaveProjectXml(
          file_path, date_groups_store_, date_entry_store_, page_setup_store_,
          title_config_store_, shape_configuration_store_,
          calendar_configuration_store_)) {
    return error;
  }
  SetFilePath(std::move(file_path));
  return std::nullopt;
}

void ProjectDocument::ImportCsv(const std::string& file_path) {
  const StateBurst burst(state_burst_topic_);
  date_entry_store_.ReceiveDateEntries(
      persistence::ReadDateEntriesFromCsv(file_path, locale_date_formatter_));
}

std::optional<std::string> ProjectDocument::ExportCsv(
    const std::string& file_path) const {
  return persistence::WriteDateEntriesToCsv(
      file_path, date_entry_store_.Get().Items(), locale_date_formatter_);
}

bool ProjectDocument::HasFilePath() const { return !file_path_.empty(); }

const std::string& ProjectDocument::FilePath() const { return file_path_; }

DateGroupStore& ProjectDocument::DateGroups() { return date_groups_store_; }

DateEntryStore& ProjectDocument::DateEntries() { return date_entry_store_; }

TransformDateEntry& ProjectDocument::Transform() {
  return transform_date_entry_;
}

PageSetupStore& ProjectDocument::PageSetup() { return page_setup_store_; }

TitleConfigStore& ProjectDocument::TitleConfiguration() {
  return title_config_store_;
}

ShapeConfigurationStore& ProjectDocument::ShapeConfiguration() {
  return shape_configuration_store_;
}

CalendarConfigStore& ProjectDocument::CalendarConfiguration() {
  return calendar_configuration_store_;
}

void ProjectDocument::SetFilePath(std::string file_path) {
  file_path_ = std::move(file_path);
  file_path_topic_(file_path_);
}

}  // namespace application
