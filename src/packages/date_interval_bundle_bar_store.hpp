#ifndef DATE_INTERVAL_BUNDLE_BAR_STORE_HPP
#define DATE_INTERVAL_BUNDLE_BAR_STORE_HPP

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "bar.hpp"
#include "date_interval_bundle.hpp"
#include "date_interval_bundle_store.hpp"
#include "detail/reentry_guard.hpp"

class DateIntervalBundleBarStore : public DateIntervalBundleStore {
 public:
  void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>&
                                      incoming_date_interval_bundles) override {
    if (Emitting()) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(Emitting());
    ProcessDateIntervalBundles(incoming_date_interval_bundles);

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

    for (const auto& bundle : GetDateIntervalBundlesInternal()) {
      const auto& interval = bundle.GetDateInterval();
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
        if (!bundle.IsExcluded()) {
          bar.SetText(std::to_string(bundle.GetNumber() + 1));
        } else {
          const std::string text_part_0 = std::to_string(bundle.GetGroup());
          const std::string text_part_1 =
              std::to_string(bundle.GetGroupNumber());

          std::string label = "E";
          label += text_part_0;
          label += "N";
          label += text_part_1;
          bar.SetText(label);
        }

        bar.SetGroup(bundle.GetGroup());
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
#endif  // DATE_INTERVAL_BUNDLE_BAR_STORE_HPP
