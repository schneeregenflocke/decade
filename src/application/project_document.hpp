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

// Das geöffnete Projekt: besitzt alle Stores, kennt seinen Dateipfad und ist
// der einzige Ort, an dem Laden und Speichern angestossen werden. Die Stores
// bekommen beim Bauen ihr Bus-Topic eingesetzt und veröffentlichen selbst.
//
// Ohne dieses Objekt reichte jede Lade- und Speicherstelle sechs Stores von
// Hand durch; jetzt steht diese Liste genau einmal hier.
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

  // Lädt ein XML-Projekt. Der Pfad wird nur bei Erfolg übernommen, damit ein
  // gescheitertes Laden nicht das Ziel des nächsten Speicherns verstellt.
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

  // Zugriff für die Composition Root, die daraus die Verdrahtung baut.
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
  // Der Pfad wechselt nur mit Laden und Speichern; die Anzeige erfährt ihn wie
  // jeden anderen Zustand über den Bus.
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
