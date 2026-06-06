#ifndef DATE_STORE_HPP
#define DATE_STORE_HPP

#include <algorithm>
#include <boost/date_time/date.hpp>
#include <boost/date_time/gregorian/formatters.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/period.hpp>
#include <boost/date_time/special_defs.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
#include <sigslot/signal.hpp>
#include <string>
#include <utility>
#include <vector>

#include "../date_utils.hpp"
#include "detail/reentry_guard.hpp"
#include "group_store.hpp"

class DateIntervalBundle {
 public:
  DateIntervalBundle()
      : date_interval_(boost::gregorian::date_period(
            boost::gregorian::date(boost::date_time::not_a_date_time),
            boost::gregorian::date(boost::date_time::not_a_date_time))),
        date_inter_interval_(boost::gregorian::date_period(
            boost::gregorian::date(boost::date_time::not_a_date_time),
            boost::gregorian::date(boost::date_time::not_a_date_time))) {}

  [[nodiscard]] const boost::gregorian::date_period& GetDateInterval() const {
    return date_interval_;
  }
  boost::gregorian::date_period& MutableDateInterval() {
    return date_interval_;
  }
  void SetDateInterval(const boost::gregorian::date_period& value) {
    date_interval_ = value;
  }

  [[nodiscard]] const boost::gregorian::date_period& GetDateInterInterval()
      const {
    return date_inter_interval_;
  }
  void SetDateInterInterval(const boost::gregorian::date_period& value) {
    date_inter_interval_ = value;
  }

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

  [[nodiscard]] int GetGroupNumber() const { return group_number_; }
  void SetGroupNumber(int group_number) { group_number_ = group_number; }

  [[nodiscard]] bool IsExcluded() const { return exclude_; }
  void SetExcluded(bool exclude) { exclude_ = exclude; }

  [[nodiscard]] const std::string& GetComment() const { return comment_; }
  void SetComment(std::string comment) { comment_ = std::move(comment); }

 private:
  boost::gregorian::date_period date_interval_;
  boost::gregorian::date_period date_inter_interval_;
  int number_{0};
  int group_{0};
  int group_number_{0};
  std::string comment_;
  bool exclude_{false};
};

namespace detail {
inline std::string ToIsoString(const boost::gregorian::date& date) {
  return boost::gregorian::to_iso_string(date);
}
}  // namespace detail

class Bar {
 public:
  explicit Bar(const boost::gregorian::date_period& date_interval)
      : date_interval_(date_interval) {}

  void SetText(const std::string& text) { text_ = text; }

  [[nodiscard]] const std::string& GetText() const { return text_; }

  [[nodiscard]] int GetYear() const { return date_interval_.begin().year(); }

  [[nodiscard]] std::int64_t GetLenght() const {
    return date_interval_.length().days();
  }

  [[nodiscard]] float GetFirstDay() const {
    return static_cast<float>(date_interval_.begin().day_of_year() - 1);
  }

  [[nodiscard]] float GetLastDay() const {
    return static_cast<float>(date_interval_.last().day_of_year());
  }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

 private:
  boost::gregorian::date_period date_interval_;
  std::string text_;
  int group_{0};
};

class DateIntervalBundleStore {
 public:
  DateIntervalBundleStore() = default;
  virtual ~DateIntervalBundleStore() = default;
  DateIntervalBundleStore(const DateIntervalBundleStore&) = delete;
  DateIntervalBundleStore& operator=(const DateIntervalBundleStore&) = delete;
  DateIntervalBundleStore(DateIntervalBundleStore&&) = delete;
  DateIntervalBundleStore& operator=(DateIntervalBundleStore&&) = delete;

  virtual void ReceiveDateIntervalBundles(
      const std::vector<DateIntervalBundle>& incoming_date_interval_bundles) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    ProcessDateIntervalBundles(incoming_date_interval_bundles);
    signal_date_interval_bundles(date_interval_bundles);
  }

  [[nodiscard]] const std::vector<DateIntervalBundle>& GetDateIntervalBundles()
      const {
    return date_interval_bundles;
  }

  [[nodiscard]] bool is_empty() const { return date_interval_bundles.empty(); }

  [[nodiscard]] std::size_t GetSpan() const {
    if (date_interval_bundles.empty()) {
      return 0;
    }
    return static_cast<std::size_t>(GetLastYear() - GetFirstYear() + 1);
  }

  [[nodiscard]] int GetFirstYear() const {
    if (date_interval_bundles.empty()) {
      return 0;
    }
    return date_interval_bundles.front().GetDateInterval().begin().year();
  }

  [[nodiscard]] int GetLastYear() const {
    if (date_interval_bundles.empty()) {
      return 0;
    }
    return date_interval_bundles.back().GetDateInterval().last().year();
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_group_store.ReceiveDateGroups(date_groups);
  }

  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalDateIntervalBundles() {
    return signal_date_interval_bundles;
  }

 protected:
  bool emitting_{false};

  [[nodiscard]] const std::vector<DateIntervalBundle>&
  GetDateIntervalBundlesInternal() const {
    return date_interval_bundles;
  }
  std::vector<DateIntervalBundle>& MutableDateIntervalBundles() {
    return date_interval_bundles;
  }

  [[nodiscard]] const DateGroupStore& GetDateGroupStore() const {
    return date_group_store;
  }
  DateGroupStore& MutableDateGroupStore() { return date_group_store; }

  void ProcessDateIntervalBundles(
      const std::vector<DateIntervalBundle>& incoming_date_interval_bundles) {
    date_interval_bundles.clear();
    date_interval_bundles.reserve(incoming_date_interval_bundles.size());

    // copy, not const reference
    for (auto date_interval_bundle : incoming_date_interval_bundles) {
      auto case_id = CheckAndAdjustDateInterval(
          &date_interval_bundle.MutableDateInterval());

      if (case_id > 0) {
        date_interval_bundles.push_back(date_interval_bundle);
      }
    }

    date_interval_bundles.shrink_to_fit();

    Sort();
    CheckAndAdjustGroupIntegrity();
    AdjustGroupExcludeFlag();
    ProcessNumbers();
    ProcessDateInterIntervals();
    ProcessDateGroupsNumber();
  }

 private:
  std::vector<DateIntervalBundle> date_interval_bundles;
  DateGroupStore date_group_store;

  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_date_interval_bundles;

  void Sort() {
    auto sort_func = [](const DateIntervalBundle& bundle0,
                        const DateIntervalBundle& bundle1) {
      return bundle0.GetDateInterval().begin() <
             bundle1.GetDateInterval().begin();
    };

    std::ranges::sort(date_interval_bundles, sort_func);
  }

  void ProcessNumbers() {
    int current_number = 0;
    for (auto& date_interval_bundle : date_interval_bundles) {
      if (!date_interval_bundle.IsExcluded()) {
        date_interval_bundle.SetNumber(current_number);
        ++current_number;
      }
    }
  }

  void ProcessDateInterIntervals() {
    auto iterator_first = date_interval_bundles.begin();
    // int counter = 0;

    while (iterator_first != date_interval_bundles.end()) {
      // std::cout << iterator_first - date_interval_bundles.begin() << '\n';

      bool loop = true;
      while (iterator_first != date_interval_bundles.end() && loop) {
        if (iterator_first->IsExcluded()) {
          ++iterator_first;
        } else {
          loop = false;
        }
      }

      if (iterator_first != date_interval_bundles.end()) {
        auto iterator_second = iterator_first + 1;

        loop = true;
        while (iterator_second != date_interval_bundles.end() && loop) {
          if (iterator_second->IsExcluded()) {
            ++iterator_second;
          } else {
            loop = false;
          }
        }

        if (iterator_second != date_interval_bundles.end()) {
          iterator_first->SetDateInterInterval(boost::gregorian::date_period(
              iterator_first->GetDateInterval().end(),
              iterator_second->GetDateInterval().begin()));

          // std::cout << "wrote inter " << counter << '\n';
          //++counter;
        }

        ++iterator_first;
      }
      // std::cout << "iterator != end? "  << (iterator_first !=
      // date_interval_bundles.end()) <<
      // '\n';
    }

    /*for (size_t index = 1; index < date_interval_bundles.size(); ++index)
    {
    date_interval_bundles[index - 1].date_inter_interval = date_period(
    date_interval_bundles[index - 1].date_interval.end(),
    date_interval_bundles[index].date_interval.begin());
    }*/
  }

  void ProcessDateGroupsNumber() {
    std::map<int, int> groups_counter;

    for (auto& date_interval_bundle : date_interval_bundles) {
      auto current_group = date_interval_bundle.GetGroup();

      if (groups_counter.contains(current_group)) {
        groups_counter[current_group] += 1;
      } else {
        groups_counter[current_group] = 0;
      }

      date_interval_bundle.SetGroupNumber(groups_counter[current_group]);
    }
  }

  void CheckAndAdjustGroupIntegrity() {
    for (auto& date_interval_bundle : date_interval_bundles) {
      if (date_interval_bundle.GetGroup() > date_group_store.GetGroupMax()) {
        date_interval_bundle.SetGroup(0);
      }
    }
  }

  void AdjustGroupExcludeFlag() {
    for (auto& bundle : date_interval_bundles) {
      bundle.SetExcluded(date_group_store.GetExclude(bundle.GetGroup()));
    }
  }
};

class DateIntervalBundleBarStore : public DateIntervalBundleStore {
 public:
  void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>&
                                      incoming_date_interval_bundles) override {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    ProcessDateIntervalBundles(incoming_date_interval_bundles);

    ProcessBars();
    ProcessAnnualTotals();
  }

  [[nodiscard]] size_t GetNumberBars() const { return bars.size(); }

  [[nodiscard]] Bar GetBar(size_t index) const { return bars[index]; }

  [[nodiscard]] std::int64_t GetAnnualTotal(size_t index) const {
    return annualTotals[index];
  }

 private:
  void ProcessBars() {
    bars.clear();

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
        bars.push_back(bar);
      }
    }
  }

  void ProcessAnnualTotals() {
    annualTotals.clear();
    annualTotals.resize(GetSpan());

    for (const auto& bar : bars) {
      const size_t annualTotalsIndex = static_cast<size_t>(bar.GetYear()) -
                                       static_cast<size_t>(GetFirstYear());

      annualTotals[annualTotalsIndex] += bar.GetLenght();
    }
  }

  std::vector<Bar> bars;
  std::vector<std::int64_t> annualTotals;
};

class TransformDateIntervalBundle {
 public:
  struct DateShift {
    int begin_days;
    int end_days;
  };

  TransformDateIntervalBundle() : date_shift{.begin_days = 0, .end_days = 0} {};

  void ReceiveDateIntervalBundles(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    std::vector<DateIntervalBundle> transformed_bundles = date_interval_bundles;
    auto bundles_iterator = transformed_bundles.begin();

    for (const auto& date_interval_bundle : date_interval_bundles) {
      const auto& interval = date_interval_bundle.GetDateInterval();
      bundles_iterator->SetDateInterval(boost::gregorian::date_period(
          interval.begin() +
              boost::gregorian::date_duration(date_shift.begin_days),
          interval.end() +
              boost::gregorian::date_duration(date_shift.end_days)));
      ++bundles_iterator;
    }

    signal_transform_date_interval_bundles(transformed_bundles);
  }

  void InputTransformedDateIntervals(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    std::vector<DateIntervalBundle> untransformed_bundles =
        date_interval_bundles;
    auto bundles_iterator = untransformed_bundles.begin();

    for (const auto& date_interval_bundle : date_interval_bundles) {
      const auto& interval = date_interval_bundle.GetDateInterval();
      bundles_iterator->SetDateInterval(boost::gregorian::date_period(
          interval.begin() -
              boost::gregorian::date_duration(date_shift.begin_days),
          interval.end() -
              boost::gregorian::date_duration(date_shift.end_days)));
      ++bundles_iterator;
    }

    signal_date_interval_bundles(untransformed_bundles);
  }

  void SetTransform(DateShift shift) { date_shift = shift; }

  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalDateIntervalBundles() {
    return signal_date_interval_bundles;
  }
  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalTransformDateIntervalBundles() {
    return signal_transform_date_interval_bundles;
  }

 private:
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_date_interval_bundles;
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_transform_date_interval_bundles;
  DateShift date_shift;
  bool emitting_{false};
};
#endif  // DATE_STORE_HPP
