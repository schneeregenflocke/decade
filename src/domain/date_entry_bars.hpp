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
// derives bars and yearly totals from it. It publishes nothing — the calendar
// reads it directly, so it needs neither a topic nor a re-entry guard.
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

  [[nodiscard]] std::int64_t GetAnnualTotal(size_t index) const;

 private:
  void ProcessBars();

  void ProcessAnnualTotals();

  DateEntryList date_entries_;
  std::vector<Bar> bars_;
  std::vector<std::int64_t> annual_totals_;
};
#endif  // DATE_ENTRY_BARS_HPP
