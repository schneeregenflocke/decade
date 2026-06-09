#ifndef DATE_ENTRY_BAR_STORE_HPP
#define DATE_ENTRY_BAR_STORE_HPP

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_entry.hpp"
#include "date_entry_store.hpp"
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
      const auto span = static_cast<std::size_t>(interval.last().year() -
                                                 interval.begin().year());

      std::vector<boost::gregorian::date_period> splitDatePeriods;
      splitDatePeriods.push_back(interval);

      for (std::size_t subIndex = 0; subIndex < span; ++subIndex) {
        const boost::gregorian::date split_date = boost::gregorian::date(
            splitDatePeriods[subIndex].begin().year() + 1, 1, 1);
        splitDatePeriods.emplace_back(split_date,
                                      splitDatePeriods[subIndex].end());
        splitDatePeriods[subIndex] = boost::gregorian::date_period(
            splitDatePeriods[subIndex].begin(), split_date);
      }

      for (const auto& split_period : splitDatePeriods) {
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
      const size_t annualTotalsIndex = static_cast<size_t>(bar.GetYear()) -
                                       static_cast<size_t>(GetFirstYear());

      annual_totals_[annualTotalsIndex] += bar.GetLenght();
    }
  }

  std::vector<Bar> bars_;
  std::vector<std::int64_t> annual_totals_;
};
#endif  // DATE_ENTRY_BAR_STORE_HPP
