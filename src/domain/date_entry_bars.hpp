#ifndef DATE_ENTRY_BARS_HPP
#define DATE_ENTRY_BARS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_entry.hpp"
#include "date_entry_list.hpp"
#include "date_group.hpp"
#include "timeline_projection.hpp"

// A read model for the drawing: it holds the same prepared entry list and
// derives from it the bars plus the days each year covers. It publishes
// nothing — the calendar reads it directly, so it needs neither a topic nor a
// re-entry guard.
class DateEntryBars {
 public:
  void ReceiveDateEntries(const std::vector<DateEntry>& incoming_date_entries);

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups);

  [[nodiscard]] bool is_empty() const;

  [[nodiscard]] std::size_t GetSpan() const;

  [[nodiscard]] int GetFirstYear() const;

  [[nodiscard]] int GetLastYear() const;

  [[nodiscard]] size_t GetNumberBars() const;

  [[nodiscard]] Bar GetBar(size_t index) const;

  // Marked days in the year at the given zero-based offset from the first one
  // — what the annual coverage bar draws and its percentage divides.
  [[nodiscard]] std::int64_t GetCoveredDays(size_t year_index) const;

 private:
  void ProcessBars();

  void ProcessCoveredDays();

  DateEntryList date_entries_;
  std::vector<Bar> bars_;
  std::vector<std::int64_t> covered_days_;
};
#endif  // DATE_ENTRY_BARS_HPP
