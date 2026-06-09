#ifndef PROJECT_IO_HPP
#define PROJECT_IO_HPP

#include <wx/platinfo.h>
#include <wx/version.h>

#include <array>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <cstddef>
#include <csv2/parameters.hpp>
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "../../packages/calendar_config.hpp"
#include "../../packages/calendar_config_store.hpp"
#include "../../packages/date_entry.hpp"
#include "../../packages/date_entry_store.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/date_group_store.hpp"
#include "../../packages/date_utils.hpp"
#include "../../packages/page_setup_config.hpp"
#include "../../packages/page_setup_store.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/shape_configuration_store.hpp"
#include "../../packages/title_config.hpp"
#include "../../packages/title_config_store.hpp"
#include "value_serialization.hpp"

namespace app::io {
using CsvReader = csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                               csv2::first_row_is_header<false>,
                               csv2::trim_policy::trim_whitespace>;

inline std::vector<DateEntry> ReadDateEntriesFromCsv(
    const std::string& file_path) {
  CsvReader csv_reader;
  std::vector<DateEntry> date_entries;
  if (!csv_reader.mmap(file_path)) {
    return date_entries;
  }

  const DateFormatDescriptor date_format = InitDateFormat();
  for (const auto& row : csv_reader) {
    size_t current_col = 0;
    std::string begin_date_string;
    std::string end_date_string;

    for (const auto& cell : row) {
      if (current_col == 0) {
        cell.read_value(begin_date_string);
      } else if (current_col == 1) {
        cell.read_value(end_date_string);
      }
      ++current_col;
    }

    const auto begin_date = StringToBoostDate(begin_date_string, date_format);
    const auto end_date = StringToBoostDate(end_date_string, date_format);

    DateEntry date_entry;
    date_entry.SetDateInterval(
        boost::gregorian::date_period(begin_date, end_date));
    date_entries.push_back(date_entry);
  }

  return date_entries;
}

inline void WriteDateEntriesToCsv(const std::string& file_path,
                                  const std::vector<DateEntry>& date_entries) {
  std::ofstream file_stream(file_path, std::ios_base::trunc);
  csv2::Writer<csv2::delimiter<','>> csv_writer(file_stream);

  for (const auto& date_entry : date_entries) {
    const std::array<std::string, 2> date_interval_strings{
        BoostDateToString(date_entry.GetDateInterval().begin()),
        BoostDateToString(date_entry.GetDateInterval().end())};
    csv_writer.write_row(date_interval_strings);
  }
}

inline void LoadProjectXml(
    const std::string& file_path, DateGroupStore& date_groups_store,
    DateEntryStore& date_entry_store, PageSetupStore& page_setup_store,
    TitleConfigStore& title_config_store,
    ShapeConfigurationStore& shape_configuration_storage,
    CalendarConfigStore& calendar_configuration_storage) {
  std::ifstream filestream(file_path);
  boost::archive::xml_iarchive iarchive(filestream);

  // Load domain values, then push them into the stores via their Receive* /
  // SetValue entry points so the change signals fire exactly as for a user
  // edit. Order matters: date groups before entries, since the entry store
  // re-derives group-dependent state on receipt.
  std::vector<DateGroup> date_groups;
  iarchive >> boost::serialization::make_nvp("date_groups", date_groups);
  date_groups_store.ReceiveDateGroups(date_groups);

  std::vector<DateEntry> date_entries;
  iarchive >> boost::serialization::make_nvp("date_entries", date_entries);
  date_entry_store.ReceiveDateEntries(date_entries);

  PageSetupConfig page_setup_config{};
  iarchive >> boost::serialization::make_nvp("page_setup", page_setup_config);
  page_setup_store.ReceivePageSetup(page_setup_config);

  TitleConfig title_config;
  iarchive >> boost::serialization::make_nvp("title_config", title_config);
  title_config_store.ReceiveTitleConfig(title_config);

  ShapeConfigSet shape_config_set;
  iarchive >> boost::serialization::make_nvp("shape_config", shape_config_set);
  shape_configuration_storage.ReceiveShapeConfigSet(shape_config_set);

  CalendarConfig calendar_config;
  iarchive >>
      boost::serialization::make_nvp("calendar_config", calendar_config);
  calendar_configuration_storage.ReceiveCalendarConfig(calendar_config);
}

inline void SaveProjectXml(
    const std::string& file_path, const DateGroupStore& date_groups_store,
    const DateEntryStore& date_entry_store,
    const PageSetupStore& page_setup_store,
    const TitleConfigStore& title_config_store,
    const ShapeConfigurationStore& shape_configuration_storage,
    const CalendarConfigStore& calendar_configuration_storage) {
  std::ofstream filestream(file_path);
  boost::archive::xml_oarchive oarchive(filestream);

  // Persist the domain values held by the stores (the stores themselves carry
  // no serialization code).
  oarchive << boost::serialization::make_nvp("date_groups",
                                             date_groups_store.GetDateGroups());
  oarchive << boost::serialization::make_nvp("date_entries",
                                             date_entry_store.GetDateEntries());
  oarchive << boost::serialization::make_nvp("page_setup",
                                             page_setup_store.GetPageSetup());
  oarchive << boost::serialization::make_nvp(
      "title_config", title_config_store.GetTitleConfig());
  oarchive << boost::serialization::make_nvp(
      "shape_config", shape_configuration_storage.GetShapeConfigSet());
  oarchive << boost::serialization::make_nvp(
      "calendar_config", calendar_configuration_storage.GetCalendarConfig());
}

inline void PrintRuntimeInfo(std::ostream& out) {
  out << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
  out << "OperatingSystemIdName "
      << wxPlatformInfo::Get().GetOperatingSystemIdName() << '\n';
  out << "ArchName " << wxPlatformInfo::Get().GetBitnessName() << '\n';
  out << "OSMajorVersion.OSMinorVersion.OSMicroVersion "
      << wxPlatformInfo::Get().GetOSMajorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMinorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMicroVersion() << '\n';

  const auto wxwidgets_version = std::wstring(wxVERSION_STRING);
  out << "wxVERSION_STRING "
      << std::string(wxwidgets_version.cbegin(), wxwidgets_version.cend())
      << '\n';
}
}  // namespace app::io

#endif  // PROJECT_IO_HPP
