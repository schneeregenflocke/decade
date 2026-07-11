#ifndef PROJECT_IO_HPP
#define PROJECT_IO_HPP

// XML project-file persistence (Infrastructure). CSV import/export lives in
// csv_io.hpp, runtime diagnostics in runtime_info.hpp.

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <exception>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "../../domain/calendar_config.hpp"
#include "../../domain/calendar_config_store.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_entry_store.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/date_group_store.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/page_setup_store.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/shape_configuration_store.hpp"
#include "../../domain/title_config.hpp"
#include "../../domain/title_config_store.hpp"
#include "value_serialization.hpp"

namespace persistence {

// Rückgabe: leer bei Erfolg, sonst die anzeigefertige Fehlermeldung. Weder
// Load noch Save lassen eine Exception entweichen — die Aufrufer sitzen in
// wx-Event-Handlern, wo ein Wurf die Anwendung abreissen würde.
[[nodiscard]] inline std::optional<std::string> LoadProjectXml(
    const std::string& file_path, DateGroupStore& date_groups_store,
    DateEntryStore& date_entry_store, PageSetupStore& page_setup_store,
    TitleConfigStore& title_config_store,
    ShapeConfigurationStore& shape_configuration_store,
    CalendarConfigStore& calendar_configuration_store) {
  std::ifstream filestream(file_path);
  if (!filestream.is_open()) {
    return "Cannot open project file: " + file_path;
  }

  // Erst die ganze Datei in lokale Werte lesen: ein Lesefehler (kaputte Datei,
  // altes, bewusst nicht mehr lesbares Format) lässt den Projektzustand so
  // unangetastet, statt die Stores halb zu überschreiben.
  std::vector<DateGroup> date_groups;
  std::vector<DateEntry> date_entries;
  PageSetupConfig page_setup_config{};
  TitleConfig title_config;
  ShapeConfigSet shape_config_set;
  CalendarConfig calendar_config;
  try {
    boost::archive::xml_iarchive iarchive(filestream);
    iarchive >> boost::serialization::make_nvp("date_groups", date_groups);
    iarchive >> boost::serialization::make_nvp("date_entries", date_entries);
    iarchive >> boost::serialization::make_nvp("page_setup", page_setup_config);
    iarchive >> boost::serialization::make_nvp("title_config", title_config);
    iarchive >>
        boost::serialization::make_nvp("shape_config", shape_config_set);
    iarchive >>
        boost::serialization::make_nvp("calendar_config", calendar_config);
  } catch (const std::exception& read_error) {
    return "Loading the project file failed: " + std::string(read_error.what());
  }

  // Die Werte über die Receive*-Eingänge in die Stores schieben, damit die
  // Änderungssignale genau wie bei einer Nutzereingabe feuern. Die Reihenfolge
  // zählt: Gruppen vor Einträgen, weil der Entry-Store beim Empfang
  // gruppenabhängigen Zustand ableitet.
  date_groups_store.ReceiveDateGroups(date_groups);
  date_entry_store.ReceiveDateEntries(date_entries);
  page_setup_store.ReceivePageSetup(page_setup_config);
  title_config_store.ReceiveTitleConfig(title_config);
  shape_configuration_store.ReceiveShapeConfigSet(shape_config_set);
  calendar_configuration_store.ReceiveCalendarConfig(calendar_config);
  return std::nullopt;
}

[[nodiscard]] inline std::optional<std::string> SaveProjectXml(
    const std::string& file_path, const DateGroupStore& date_groups_store,
    const DateEntryStore& date_entry_store,
    const PageSetupStore& page_setup_store,
    const TitleConfigStore& title_config_store,
    const ShapeConfigurationStore& shape_configuration_store,
    const CalendarConfigStore& calendar_configuration_store) {
  std::ofstream filestream(file_path);
  if (!filestream.is_open()) {
    return "Cannot open file for writing: " + file_path;
  }

  try {
    // Der Archiv-Destruktor schreibt den XML-Abschluss — deshalb der eigene
    // Scope vor der Stream-Prüfung. Die Stores selbst tragen keinen
    // Serialisierungscode; persistiert werden ihre Domain-Werte.
    boost::archive::xml_oarchive oarchive(filestream);
    oarchive << boost::serialization::make_nvp(
        "date_groups", date_groups_store.GetDateGroups());
    oarchive << boost::serialization::make_nvp(
        "date_entries", date_entry_store.GetDateEntries());
    oarchive << boost::serialization::make_nvp("page_setup",
                                               page_setup_store.GetPageSetup());
    oarchive << boost::serialization::make_nvp(
        "title_config", title_config_store.GetTitleConfig());
    oarchive << boost::serialization::make_nvp(
        "shape_config", shape_configuration_store.GetShapeConfigSet());
    oarchive << boost::serialization::make_nvp(
        "calendar_config", calendar_configuration_store.GetCalendarConfig());
  } catch (const std::exception& write_error) {
    return "Saving the project file failed: " +
           std::string(write_error.what());
  }

  filestream.flush();
  if (!filestream.good()) {
    return "Writing to " + file_path + " failed.";
  }
  return std::nullopt;
}

}  // namespace persistence

#endif  // PROJECT_IO_HPP
