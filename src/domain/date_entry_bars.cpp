#include "date_entry_bars.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_entry.hpp"
#include "date_group.hpp"
#include "timeline_projection.hpp"

void DateEntryBars::ReceiveDateEntries(
    const std::vector<DateEntry>& incoming_date_entries) {
  date_entries_.Assign(incoming_date_entries);
  ProcessBars();
  ProcessAnnualTotals();
}

void DateEntryBars::ReceiveDateGroups(
    const std::vector<DateGroup>& date_groups) {
  date_entries_.AssignDateGroups(date_groups);
}

bool DateEntryBars::is_empty() const { return date_entries_.IsEmpty(); }

std::size_t DateEntryBars::GetSpan() const { return date_entries_.YearSpan(); }

int DateEntryBars::GetFirstYear() const { return date_entries_.FirstYear(); }

int DateEntryBars::GetLastYear() const { return date_entries_.LastYear(); }

size_t DateEntryBars::GetNumberBars() const { return bars_.size(); }

Bar DateEntryBars::GetBar(size_t index) const { return bars_[index]; }

std::int64_t DateEntryBars::GetAnnualTotal(size_t index) const {
  return annual_totals_[index];
}

void DateEntryBars::ProcessBars() {
  bars_.clear();

  for (const auto& entry : date_entries_.Items()) {
    // Stored periods are never null (filtered upstream), so the row-period
    // split is well-defined. The split rule (one bar per calendar year)
    // lives in the domain projection, not in this store.
    const auto split_date_periods =
        SplitAtYearBoundaries(entry.GetDateInterval());

    for (const auto& split_period : split_date_periods) {
      Bar bar(split_period);
      bar.SetText(std::to_string(entry.GetNumber() + 1));
      bar.SetGroup(entry.GetGroup());
      bars_.push_back(bar);
    }
  }
}

void DateEntryBars::ProcessAnnualTotals() {
  annual_totals_.clear();
  annual_totals_.resize(GetSpan());

  for (const auto& bar : bars_) {
    const size_t annual_totals_index = static_cast<size_t>(bar.GetYear()) -
                                       static_cast<size_t>(GetFirstYear());

    annual_totals_[annual_totals_index] += bar.GetLength();
  }
}
