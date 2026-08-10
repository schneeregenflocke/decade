#include "project_io.hpp"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/core/nvp.hpp>
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
// The archive operators instantiate the save/load overloads from here; the
// include check cannot see a use it resolves through template instantiation.
#include "value_serialization.hpp"  // IWYU pragma: keep

namespace persistence {

std::optional<std::string> LoadProjectXml(
    const std::string& file_path, DateGroupStore& date_groups_store,
    DateEntryStore& date_entry_store, PageSetupStore& page_setup_store,
    TitleConfigStore& title_config_store,
    ShapeConfigurationStore& shape_configuration_store,
    CalendarConfigStore& calendar_configuration_store) {
  std::ifstream filestream(file_path);
  if (!filestream.is_open()) {
    return "Cannot open project file: " + file_path;
  }

  // Read the whole file into local values first: that way a read error (a
  // broken file, an old format deliberately no longer readable) leaves the
  // project state untouched instead of half-overwriting the stores.
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

  // Push the values into the stores through the Receive* inputs, so the change
  // signals fire exactly as on user input. The order counts: groups before
  // entries, because the entry store derives group-dependent state on receipt.
  date_groups_store.ReceiveDateGroups(date_groups);
  date_entry_store.ReceiveDateEntries(date_entries);
  page_setup_store.ReceivePageSetup(page_setup_config);
  title_config_store.ReceiveTitleConfig(title_config);
  shape_configuration_store.ReceiveShapeConfigSet(shape_config_set);
  calendar_configuration_store.ReceiveCalendarConfig(calendar_config);
  return std::nullopt;
}

std::optional<std::string> SaveProjectXml(
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
    // The archive destructor writes the XML closing — hence the scope of its
    // own before the stream check. The stores carry no serialisation code
    // themselves; what gets persisted are their domain values.
    boost::archive::xml_oarchive oarchive(filestream);
    oarchive << boost::serialization::make_nvp("date_groups",
                                               date_groups_store.Get().Items());
    oarchive << boost::serialization::make_nvp("date_entries",
                                               date_entry_store.Get().Items());
    oarchive << boost::serialization::make_nvp("page_setup",
                                               page_setup_store.Get());
    oarchive << boost::serialization::make_nvp("title_config",
                                               title_config_store.Get());
    oarchive << boost::serialization::make_nvp("shape_config",
                                               shape_configuration_store.Get());
    oarchive << boost::serialization::make_nvp(
        "calendar_config", calendar_configuration_store.Get());
  } catch (const std::exception& write_error) {
    return "Saving the project file failed: " + std::string(write_error.what());
  }

  filestream.flush();
  if (!filestream.good()) {
    return "Writing to " + file_path + " failed.";
  }
  return std::nullopt;
}

}  // namespace persistence
