#include "project_io.hpp"

#include <array>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <csv2/parameters.hpp>
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>

#include <wx/platinfo.h>
#include <wx/version.h>

#include "../../date_utils.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date_store.hpp"
#include "../../packages/group_store.hpp"
#include "../../packages/page_config.hpp"
#include "../../packages/shape_config.hpp"
#include "../../packages/title_config.hpp"

namespace app::io {
namespace {
using CsvReader =
    csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                 csv2::first_row_is_header<false>, csv2::trim_policy::trim_whitespace>;
} // namespace

std::vector<DateIntervalBundle> ReadDateIntervalBundlesFromCsv(const std::string &file_path)
{
  CsvReader csv_reader;
  std::vector<DateIntervalBundle> date_interval_bundles;
  if (!csv_reader.mmap(file_path)) {
    return date_interval_bundles;
  }

  const date_format_descriptor date_format = InitDateFormat();
  for (const auto &row : csv_reader) {
    size_t current_col = 0;
    std::string begin_date_string;
    std::string end_date_string;

    for (const auto &cell : row) {
      if (current_col == 0) {
        cell.read_value(begin_date_string);
      } else if (current_col == 1) {
        cell.read_value(end_date_string);
      }
      ++current_col;
    }

    const auto begin_date = string_to_boost_date(begin_date_string, date_format);
    const auto end_date = string_to_boost_date(end_date_string, date_format);

    DateIntervalBundle date_interval_bundle;
    date_interval_bundle.SetDateInterval(boost::gregorian::date_period(begin_date, end_date));
    date_interval_bundles.push_back(date_interval_bundle);
  }

  return date_interval_bundles;
}

void WriteDateIntervalBundlesToCsv(const std::string &file_path,
                                   const std::vector<DateIntervalBundle> &date_interval_bundles)
{
  std::ofstream file_stream(file_path, std::ios_base::trunc);
  csv2::Writer<csv2::delimiter<','>> csv_writer(file_stream);

  for (const auto &date_interval_bundle : date_interval_bundles) {
    const std::array<std::string, 2> date_interval_strings{
        boost_date_to_string(date_interval_bundle.GetDateInterval().begin()),
        boost_date_to_string(date_interval_bundle.GetDateInterval().end())};
    csv_writer.write_row(date_interval_strings);
  }
}

void LoadProjectXml(const std::string &file_path, DateGroupStore &date_groups_store,
                    DateIntervalBundleStore &date_interval_bundle_store,
                    PageSetupStore &page_setup_store, TitleConfigStore &title_config_store,
                    ShapeConfigurationStorage &shape_configuration_storage,
                    CalendarConfigStorage &calendar_configuration_storage)
{
  std::ifstream filestream(file_path);
  boost::archive::xml_iarchive iarchive(filestream);

  iarchive >> BOOST_SERIALIZATION_NVP(date_groups_store);
  iarchive >> BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
  iarchive >> BOOST_SERIALIZATION_NVP(page_setup_store);
  iarchive >> BOOST_SERIALIZATION_NVP(title_config_store);
  iarchive >> BOOST_SERIALIZATION_NVP(shape_configuration_storage);
  iarchive >> BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
}

void SaveProjectXml(const std::string &file_path, const DateGroupStore &date_groups_store,
                    const DateIntervalBundleStore &date_interval_bundle_store,
                    const PageSetupStore &page_setup_store,
                    const TitleConfigStore &title_config_store,
                    const ShapeConfigurationStorage &shape_configuration_storage,
                    const CalendarConfigStorage &calendar_configuration_storage)
{
  std::ofstream filestream(file_path);
  boost::archive::xml_oarchive oarchive(filestream);

  oarchive << BOOST_SERIALIZATION_NVP(date_groups_store);
  oarchive << BOOST_SERIALIZATION_NVP(date_interval_bundle_store);
  oarchive << BOOST_SERIALIZATION_NVP(page_setup_store);
  oarchive << BOOST_SERIALIZATION_NVP(title_config_store);
  oarchive << BOOST_SERIALIZATION_NVP(shape_configuration_storage);
  oarchive << BOOST_SERIALIZATION_NVP(calendar_configuration_storage);
}

void PrintRuntimeInfo(std::ostream &out)
{
  out << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
  out << "OperatingSystemIdName " << wxPlatformInfo::Get().GetOperatingSystemIdName() << '\n';
  out << "ArchName " << wxPlatformInfo::Get().GetBitnessName() << '\n';
  out << "OSMajorVersion.OSMinorVersion.OSMicroVersion "
      << wxPlatformInfo::Get().GetOSMajorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMinorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMicroVersion() << '\n';

  const auto wxwidgets_version = std::wstring(wxVERSION_STRING);
  out << "wxVERSION_STRING "
      << std::string(wxwidgets_version.cbegin(), wxwidgets_version.cend()) << '\n';
}
} // namespace app::io
