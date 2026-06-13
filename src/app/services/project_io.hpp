#ifndef PROJECT_IO_HPP
#define PROJECT_IO_HPP

// XML project-file persistence (Infrastructure). CSV import/export lives in
// csv_io.hpp, runtime diagnostics in runtime_info.hpp.

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <fstream>
#include <string>
#include <vector>

#include "../../packages/calendar_config.hpp"
#include "../../packages/calendar_config_store.hpp"
#include "../../packages/date_entry.hpp"
#include "../../packages/date_entry_store.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/date_group_store.hpp"
#include "../../packages/page_setup_config.hpp"
#include "../../packages/page_setup_store.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/shape_configuration_store.hpp"
#include "../../packages/title_config.hpp"
#include "../../packages/title_config_store.hpp"
#include "value_serialization.hpp"

namespace app::io {

inline void LoadProjectXml(const std::string& file_path,
                           DateGroupStore& date_groups_store,
                           DateEntryStore& date_entry_store,
                           PageSetupStore& page_setup_store,
                           TitleConfigStore& title_config_store,
                           ShapeConfigurationStore& shape_configuration_store,
                           CalendarConfigStore& calendar_configuration_store) {
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
  shape_configuration_store.ReceiveShapeConfigSet(shape_config_set);

  CalendarConfig calendar_config;
  iarchive >>
      boost::serialization::make_nvp("calendar_config", calendar_config);
  calendar_configuration_store.ReceiveCalendarConfig(calendar_config);
}

inline void SaveProjectXml(
    const std::string& file_path, const DateGroupStore& date_groups_store,
    const DateEntryStore& date_entry_store,
    const PageSetupStore& page_setup_store,
    const TitleConfigStore& title_config_store,
    const ShapeConfigurationStore& shape_configuration_store,
    const CalendarConfigStore& calendar_configuration_store) {
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
      "shape_config", shape_configuration_store.GetShapeConfigSet());
  oarchive << boost::serialization::make_nvp(
      "calendar_config", calendar_configuration_store.GetCalendarConfig());
}

}  // namespace app::io

#endif  // PROJECT_IO_HPP
