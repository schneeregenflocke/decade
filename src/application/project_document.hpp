#ifndef PROJECT_DOCUMENT_HPP
#define PROJECT_DOCUMENT_HPP

#include <optional>
#include <string>
#include <utility>

#include "../domain/calendar_config_store.hpp"
#include "../domain/date_entry_store.hpp"
#include "../domain/date_format.hpp"
#include "../domain/date_group_store.hpp"
#include "../domain/page_setup_store.hpp"
#include "../domain/shape_configuration_store.hpp"
#include "../domain/state_topic.hpp"
#include "../domain/title_config_store.hpp"
#include "../domain/transform_date_entry.hpp"
#include "../infrastructure/persistence/csv_io.hpp"
#include "../infrastructure/persistence/project_io.hpp"
#include "event_bus.hpp"

namespace application {

// The open project: it owns every store, knows its file path and is the only
// place loading and saving get triggered from. The stores get their bus topic
// injected on construction and publish themselves.
//
// Without this object every load and save site passed six stores through by
// hand; now that list stands exactly once, here.
class ProjectDocument {
 public:
  explicit ProjectDocument(EventBus& bus,
                           LocaleDateFormatter& locale_date_formatter)
      : locale_date_formatter_(locale_date_formatter),
        file_path_topic_(bus.project_file_path()),
        date_groups_store_(bus.date_groups()),
        date_entry_store_(bus.date_entries()),
        transform_date_entry_(bus.transformed_date_entries()),
        page_setup_store_(bus.page_setup()),
        title_config_store_(bus.title_config()),
        shape_configuration_store_(bus.shape_config_set()),
        calendar_configuration_store_(bus.calendar_config()) {}

  ~ProjectDocument() = default;
  ProjectDocument(const ProjectDocument&) = delete;
  ProjectDocument& operator=(const ProjectDocument&) = delete;
  ProjectDocument(ProjectDocument&&) = delete;
  ProjectDocument& operator=(ProjectDocument&&) = delete;

  // Loads an XML project. The path gets taken over on success alone, so a
  // failed load does not misplace the target of the next save.
  [[nodiscard]] std::optional<std::string> LoadXml(std::string file_path) {
    if (auto error = persistence::LoadProjectXml(
            file_path, date_groups_store_, date_entry_store_, page_setup_store_,
            title_config_store_, shape_configuration_store_,
            calendar_configuration_store_)) {
      return error;
    }
    SetFilePath(std::move(file_path));
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string> SaveXml(std::string file_path) {
    if (auto error = persistence::SaveProjectXml(
            file_path, date_groups_store_, date_entry_store_, page_setup_store_,
            title_config_store_, shape_configuration_store_,
            calendar_configuration_store_)) {
      return error;
    }
    SetFilePath(std::move(file_path));
    return std::nullopt;
  }

  void ImportCsv(const std::string& file_path) {
    date_entry_store_.ReceiveDateEntries(
        persistence::ReadDateEntriesFromCsv(file_path, locale_date_formatter_));
  }

  [[nodiscard]] std::optional<std::string> ExportCsv(
      const std::string& file_path) const {
    return persistence::WriteDateEntriesToCsv(
        file_path, date_entry_store_.Get().Items(), locale_date_formatter_);
  }

  [[nodiscard]] bool HasFilePath() const { return !file_path_.empty(); }
  [[nodiscard]] const std::string& FilePath() const { return file_path_; }

  // Access for the composition root, which builds the wiring out of it.
  [[nodiscard]] DateGroupStore& DateGroups() { return date_groups_store_; }
  [[nodiscard]] DateEntryStore& DateEntries() { return date_entry_store_; }
  [[nodiscard]] TransformDateEntry& Transform() {
    return transform_date_entry_;
  }
  [[nodiscard]] PageSetupStore& PageSetup() { return page_setup_store_; }
  [[nodiscard]] TitleConfigStore& TitleConfiguration() {
    return title_config_store_;
  }
  [[nodiscard]] ShapeConfigurationStore& ShapeConfiguration() {
    return shape_configuration_store_;
  }
  [[nodiscard]] CalendarConfigStore& CalendarConfiguration() {
    return calendar_configuration_store_;
  }

 private:
  // The path changes with loading and saving alone; the display learns it over
  // the bus like any other state.
  void SetFilePath(std::string file_path) {
    file_path_ = std::move(file_path);
    file_path_topic_(file_path_);
  }

  LocaleDateFormatter& locale_date_formatter_;
  domain::StateTopic<std::string>& file_path_topic_;
  std::string file_path_;

  DateGroupStore date_groups_store_;
  DateEntryStore date_entry_store_;
  TransformDateEntry transform_date_entry_;
  PageSetupStore page_setup_store_;
  TitleConfigStore title_config_store_;
  ShapeConfigurationStore shape_configuration_store_;
  CalendarConfigStore calendar_configuration_store_;
};

}  // namespace application

#endif  // PROJECT_DOCUMENT_HPP
