#include "date_entry_bars.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_category.hpp"
#include "date_entry.hpp"
#include "timeline_projection.hpp"

void DateEntryBars::ReceiveDateEntries(
    const std::vector<DateEntry>& incoming_date_entries) {
  date_entries_.Assign(incoming_date_entries);
  ProcessBars();
  ProcessCoveredDays();
}

void DateEntryBars::ReceiveDateCategories(
    const std::vector<DateCategory>& date_categories) {
  date_entries_.AssignDateCategories(date_categories);
}

bool DateEntryBars::is_empty() const { return date_entries_.IsEmpty(); }

std::size_t DateEntryBars::GetSpan() const { return date_entries_.YearSpan(); }

int DateEntryBars::GetFirstYear() const { return date_entries_.FirstYear(); }

int DateEntryBars::GetLastYear() const { return date_entries_.LastYear(); }

size_t DateEntryBars::GetNumberBars() const { return bars_.size(); }

const Bar& DateEntryBars::GetBar(size_t index) const { return bars_[index]; }

std::int64_t DateEntryBars::GetCoveredDays(size_t year_index) const {
  return covered_days_[year_index];
}

void DateEntryBars::ProcessBars() {
  bars_.clear();

  for (const auto& entry : date_entries_.Items()) {
    Bar bar(entry.GetDateInterval());
    bar.SetText(std::to_string(entry.GetNumber() + 1));
    bar.SetCategory(entry.GetCategory());
    bars_.push_back(bar);
  }
}

void DateEntryBars::ProcessCoveredDays() {
  covered_days_.clear();
  covered_days_.resize(GetSpan());

  for (const auto& bar : bars_) {
    // Stored periods are never null (filtered upstream), so the split is
    // well-defined.
    for (const auto& year_part : SplitAtYearBoundaries(bar.Period())) {
      const auto covered_days_index =
          static_cast<size_t>(year_part.Begin().Year() - GetFirstYear());
      covered_days_[covered_days_index] += year_part.LengthDays();
    }
  }
}
