#ifndef DATE_INTERVAL_BUNDLE_STORE_HPP
#define DATE_INTERVAL_BUNDLE_STORE_HPP

#include <algorithm>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cstddef>
#include <map>
#include <sigslot/signal.hpp>
#include <vector>

#include "date_group.hpp"
#include "date_group_store.hpp"
#include "date_interval_bundle.hpp"
#include "date_utils.hpp"
#include "detail/reentry_guard.hpp"

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
    signal_date_interval_bundles_(date_interval_bundles_);
  }

  [[nodiscard]] const std::vector<DateIntervalBundle>& GetDateIntervalBundles()
      const {
    return date_interval_bundles_;
  }

  [[nodiscard]] bool is_empty() const { return date_interval_bundles_.empty(); }

  [[nodiscard]] std::size_t GetSpan() const {
    if (date_interval_bundles_.empty()) {
      return 0;
    }
    const int year_span = GetLastYear() - GetFirstYear() + 1;
    return static_cast<std::size_t>(year_span);
  }

  [[nodiscard]] int GetFirstYear() const {
    if (date_interval_bundles_.empty()) {
      return 0;
    }
    return date_interval_bundles_.front().GetDateInterval().begin().year();
  }

  [[nodiscard]] int GetLastYear() const {
    if (date_interval_bundles_.empty()) {
      return 0;
    }
    return date_interval_bundles_.back().GetDateInterval().last().year();
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_group_store_.ReceiveDateGroups(date_groups);
  }

  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalDateIntervalBundles() {
    return signal_date_interval_bundles_;
  }

 protected:
  // Re-entry guard flag, owned privately but reachable by subclasses through
  // this accessor (the flag itself stays private to keep data encapsulated).
  [[nodiscard]] bool& Emitting() { return emitting_; }

  [[nodiscard]] const std::vector<DateIntervalBundle>&
  GetDateIntervalBundlesInternal() const {
    return date_interval_bundles_;
  }
  std::vector<DateIntervalBundle>& MutableDateIntervalBundles() {
    return date_interval_bundles_;
  }

  [[nodiscard]] const DateGroupStore& GetDateGroupStore() const {
    return date_group_store_;
  }
  DateGroupStore& MutableDateGroupStore() { return date_group_store_; }

  void ProcessDateIntervalBundles(
      const std::vector<DateIntervalBundle>& incoming_date_interval_bundles) {
    date_interval_bundles_.clear();
    date_interval_bundles_.reserve(incoming_date_interval_bundles.size());

    // copy, not const reference
    for (auto date_interval_bundle : incoming_date_interval_bundles) {
      auto case_id = CheckAndAdjustDateInterval(
          &date_interval_bundle.MutableDateInterval());

      if (case_id > 0) {
        date_interval_bundles_.push_back(date_interval_bundle);
      }
    }

    date_interval_bundles_.shrink_to_fit();

    Sort();
    CheckAndAdjustGroupIntegrity();
    AdjustGroupExcludeFlag();
    ProcessNumbers();
    ProcessDateInterIntervals();
    ProcessDateGroupsNumber();
  }

 private:
  bool emitting_{false};
  std::vector<DateIntervalBundle> date_interval_bundles_;
  DateGroupStore date_group_store_;

  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_date_interval_bundles_;

  void Sort() {
    auto sort_func = [](const DateIntervalBundle& bundle0,
                        const DateIntervalBundle& bundle1) {
      return bundle0.GetDateInterval().begin() <
             bundle1.GetDateInterval().begin();
    };

    std::ranges::sort(date_interval_bundles_, sort_func);
  }

  void ProcessNumbers() {
    int current_number = 0;
    for (auto& date_interval_bundle : date_interval_bundles_) {
      if (!date_interval_bundle.IsExcluded()) {
        date_interval_bundle.SetNumber(current_number);
        ++current_number;
      }
    }
  }

  void ProcessDateInterIntervals() {
    auto iterator_first = date_interval_bundles_.begin();

    while (iterator_first != date_interval_bundles_.end()) {
      bool loop = true;
      while (iterator_first != date_interval_bundles_.end() && loop) {
        if (iterator_first->IsExcluded()) {
          ++iterator_first;
        } else {
          loop = false;
        }
      }

      if (iterator_first != date_interval_bundles_.end()) {
        auto iterator_second = iterator_first + 1;

        loop = true;
        while (iterator_second != date_interval_bundles_.end() && loop) {
          if (iterator_second->IsExcluded()) {
            ++iterator_second;
          } else {
            loop = false;
          }
        }

        if (iterator_second != date_interval_bundles_.end()) {
          iterator_first->SetDateInterInterval(boost::gregorian::date_period(
              iterator_first->GetDateInterval().end(),
              iterator_second->GetDateInterval().begin()));
        }

        ++iterator_first;
      }
    }
  }

  void ProcessDateGroupsNumber() {
    std::map<int, int> groups_counter;

    for (auto& date_interval_bundle : date_interval_bundles_) {
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
    for (auto& date_interval_bundle : date_interval_bundles_) {
      if (date_interval_bundle.GetGroup() > date_group_store_.GetGroupMax()) {
        date_interval_bundle.SetGroup(0);
      }
    }
  }

  void AdjustGroupExcludeFlag() {
    for (auto& bundle : date_interval_bundles_) {
      bundle.SetExcluded(date_group_store_.GetExclude(bundle.GetGroup()));
    }
  }
};
#endif  // DATE_INTERVAL_BUNDLE_STORE_HPP
