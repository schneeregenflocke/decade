#include "csv_io.hpp"

#include <array>
#include <cstddef>
#include <csv2/parameters.hpp>
#include <csv2/reader.hpp>
#include <csv2/writer.hpp>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "../../domain/date_entry.hpp"
#include "../../domain/date_format.hpp"
#include "../../domain/date_period.hpp"

namespace persistence {
namespace {

using CsvReader = csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'"'>,
                               csv2::first_row_is_header<false>,
                               csv2::trim_policy::trim_whitespace>;

}  // namespace

std::vector<DateEntry> ReadDateEntriesFromCsv(
    const std::string& file_path, LocaleDateFormatter& date_format) {
  std::vector<DateEntry> date_entries;

  // csv2::Reader::mmap maps the file through its constructor, which throws a
  // std::system_error for a missing or empty file rather than returning false.
  // Guard against that here so a bad path yields an empty result, not an
  // exception.
  std::error_code error;
  if (!std::filesystem::is_regular_file(file_path, error) ||
      std::filesystem::file_size(file_path, error) == 0) {
    return date_entries;
  }

  CsvReader csv_reader;
  if (!csv_reader.mmap(file_path)) {
    return date_entries;
  }

  for (const auto& row : csv_reader) {
    size_t current_col = 0;
    std::string begin_date_string;
    std::string last_date_string;

    for (const auto& cell : row) {
      if (current_col == 0) {
        cell.read_value(begin_date_string);
      } else if (current_col == 1) {
        cell.read_value(last_date_string);
      }
      ++current_col;
    }

    // Skip blank lines (including the trailing empty line csv2 reports for a
    // file ending in a newline). A row with any content is kept; an
    // unparseable one becomes a null period that the store drops downstream.
    if (begin_date_string.empty() && last_date_string.empty()) {
      continue;
    }

    const auto begin_date = date_format.Parse(begin_date_string);
    const auto last_date = date_format.Parse(last_date_string);

    DateEntry date_entry;
    date_entry.SetDateInterval(PeriodFromInclusiveDates(begin_date, last_date));
    date_entries.push_back(date_entry);
  }

  return date_entries;
}

std::optional<std::string> WriteDateEntriesToCsv(
    const std::string& file_path, const std::vector<DateEntry>& date_entries,
    LocaleDateFormatter& date_format) {
  std::ofstream file_stream(file_path, std::ios_base::trunc);
  if (!file_stream.is_open()) {
    return "Cannot open file for writing: " + file_path;
  }

  {
    // csv2::Writer closes the stream in its destructor — hence the scope of its
    // own before the state check.
    csv2::Writer<csv2::delimiter<','>> csv_writer(file_stream);

    for (const auto& date_entry : date_entries) {
      const std::array<std::string, 2> date_interval_strings{
          date_format.Format(date_entry.GetDateInterval().Begin()),
          date_format.Format(date_entry.GetDateInterval().Last())};
      csv_writer.write_row(date_interval_strings);
    }
  }

  if (!file_stream.good()) {
    return "Writing to " + file_path + " failed.";
  }
  return std::nullopt;
}

}  // namespace persistence
