#ifndef PROJECT_IO_HPP
#define PROJECT_IO_HPP

#include <iosfwd>
#include <string>
#include <vector>

#include <boost/serialization/split_free.hpp>

#include "../../packages/calendar_config.hpp"
#include "../../packages/date_store.hpp"
#include "../../packages/group_store.hpp"
#include "../../packages/page_config.hpp"
#include "../../packages/shape_config.hpp"
#include "../../packages/title_config.hpp"

namespace app::io {
std::vector<DateIntervalBundle> ReadDateIntervalBundlesFromCsv(const std::string &file_path);

void WriteDateIntervalBundlesToCsv(const std::string &file_path,
                                   const std::vector<DateIntervalBundle> &date_interval_bundles);

void LoadProjectXml(const std::string &file_path, DateGroupStore &date_groups_store,
                    DateIntervalBundleStore &date_interval_bundle_store,
                    PageSetupStore &page_setup_store, TitleConfigStore &title_config_store,
                    ShapeConfigurationStorage &shape_configuration_storage,
                    CalendarConfigStorage &calendar_configuration_storage);

void SaveProjectXml(const std::string &file_path, const DateGroupStore &date_groups_store,
                    const DateIntervalBundleStore &date_interval_bundle_store,
                    const PageSetupStore &page_setup_store,
                    const TitleConfigStore &title_config_store,
                    const ShapeConfigurationStorage &shape_configuration_storage,
                    const CalendarConfigStorage &calendar_configuration_storage);

void PrintRuntimeInfo(std::ostream &out);
} // namespace app::io

#endif // PROJECT_IO_HPP
