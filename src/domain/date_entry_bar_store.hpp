#ifndef DATE_ENTRY_BAR_STORE_HPP
#define DATE_ENTRY_BAR_STORE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_entry.hpp"
#include "date_entry_store.hpp"
#include "detail/reentry_guard.hpp"
#include "timeline_projection.hpp"

class DateEntryBarStore : public DateEntryStore {
 public:
  void ReceiveDateEntries(
      const std::vector<DateEntry>& incoming_date_entries) override {
    if (Emitting()) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(Emitting());
    ProcessDateEntries(incoming_date_entries);

    ProcessBars();
    ProcessAnnualTotals();
  }

  [[nodiscard]] size_t GetNumberBars() const { return bars_.size(); }

  [[nodiscard]] Bar GetBar(size_t index) const { return bars_[index]; }

  [[nodiscard]] std::int64_t GetAnnualTotal(size_t index) const {
    return annual_totals_[index];
  }

 private:
  void ProcessBars() {
    bars_.clear();

    for (const auto& entry : GetDateEntriesInternal()) {
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

  void ProcessAnnualTotals() {
    annual_totals_.clear();
    annual_totals_.resize(GetSpan());

    for (const auto& bar : bars_) {
      const size_t annual_totals_index = static_cast<size_t>(bar.GetYear()) -
                                         static_cast<size_t>(GetFirstYear());

      annual_totals_[annual_totals_index] += bar.GetLength();
    }
  }

  std::vector<Bar> bars_;
  std::vector<std::int64_t> annual_totals_;
};
#endif  // DATE_ENTRY_BAR_STORE_HPP
