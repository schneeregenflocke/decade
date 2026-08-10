#ifndef CSV_IO_HPP
#define CSV_IO_HPP

// CSV import/export for date entries (Infrastructure). Deliberately wx-free so
// the conversion logic is unit-testable.
//
// CSV files are user-facing data: each row is "from,to" with the to-date
// inclusive, formatted in the formatter's locale. Like the date table, this is
// a boundary where the inclusive user form is converted to the internal
// half-open period (PeriodFromInclusiveDates on read, Last() on write).

#include <optional>
#include <string>
#include <vector>

#include "../../domain/date_entry.hpp"
#include "../../domain/date_format.hpp"

namespace persistence {

std::vector<DateEntry> ReadDateEntriesFromCsv(const std::string& file_path,
                                              LocaleDateFormatter& date_format);

// The return: empty on success, otherwise the display-ready error message — as
// with the project I/O. An export that failed unnoticed would look to the user
// like a successful one.
[[nodiscard]] std::optional<std::string> WriteDateEntriesToCsv(
    const std::string& file_path, const std::vector<DateEntry>& date_entries,
    LocaleDateFormatter& date_format);

}  // namespace persistence

#endif  // CSV_IO_HPP
