#ifndef DATE_ENTRY_BAR_STORE_HPP
#define DATE_ENTRY_BAR_STORE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date.hpp"
#include "date_entry.hpp"
#include "date_entry_store.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"

class DateEntryBarStore : public DateEntryStore {
 public:
  void ReceiveDateEntries(
      const std::vector<DateEntry>& incoming_date_entries) override {
    if (Emitting()) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(Emitting());
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
      const auto& interval = entry.GetDateInterval();
      // Stored periods are never null (filtered upstream), so Last() >=
      // Begin() and the year difference cannot go negative.
      const auto span = static_cast<std::size_t>(interval.Last().Year() -
                                                 interval.Begin().Year());

      std::vector<DatePeriod> split_date_periods;
      split_date_periods.push_back(interval);

      for (std::size_t sub_index = 0; sub_index < span; ++sub_index) {
        const Date split_date = Date::FromYmd(
            split_date_periods[sub_index].Begin().Year() + 1, 1, 1);
        split_date_periods.emplace_back(split_date,
                                        split_date_periods[sub_index].End());
        split_date_periods[sub_index] =
            DatePeriod(split_date_periods[sub_index].Begin(), split_date);
      }

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
