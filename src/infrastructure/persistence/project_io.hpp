#ifndef PROJECT_IO_HPP
#define PROJECT_IO_HPP

// XML project-file persistence (Infrastructure). CSV import/export lives in
// csv_io.hpp, runtime diagnostics in runtime_info.hpp.

#include <optional>
#include <string>

#include "../../domain/calendar_config_store.hpp"
#include "../../domain/date_entry_store.hpp"
#include "../../domain/date_group_store.hpp"
#include "../../domain/page_setup_store.hpp"
#include "../../domain/shape_configuration_store.hpp"
#include "../../domain/title_config_store.hpp"

namespace persistence {

// The return: empty on success, otherwise the display-ready error message.
// Neither Load nor Save lets an exception escape — the callers sit in wx event
// handlers, where a throw would tear the application down.
[[nodiscard]] std::optional<std::string> LoadProjectXml(
    const std::string& file_path, DateGroupStore& date_groups_store,
    DateEntryStore& date_entry_store, PageSetupStore& page_setup_store,
    TitleConfigStore& title_config_store,
    ShapeConfigurationStore& shape_configuration_store,
    CalendarConfigStore& calendar_configuration_store);

[[nodiscard]] std::optional<std::string> SaveProjectXml(
    const std::string& file_path, const DateGroupStore& date_groups_store,
    const DateEntryStore& date_entry_store,
    const PageSetupStore& page_setup_store,
    const TitleConfigStore& title_config_store,
    const ShapeConfigurationStore& shape_configuration_store,
    const CalendarConfigStore& calendar_configuration_store);

}  // namespace persistence

#endif  // PROJECT_IO_HPP
