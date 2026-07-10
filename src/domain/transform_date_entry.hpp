#ifndef TRANSFORM_DATE_ENTRY_HPP
#define TRANSFORM_DATE_ENTRY_HPP

#include <sigslot/signal.hpp>
#include <vector>

#include "date_entry.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"

class TransformDateEntry {
 public:
  struct DateShift {
    int begin_days;
    int end_days;
  };

  TransformDateEntry() : date_shift_{.begin_days = 0, .end_days = 0} {};

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    std::vector<DateEntry> transformed_entries = date_entries;
    auto entries_iterator = transformed_entries.begin();

    for (const auto& date_entry : date_entries) {
      const auto& interval = date_entry.GetDateInterval();
      entries_iterator->SetDateInterval(
          DatePeriod(interval.Begin().AddDays(date_shift_.begin_days),
                     interval.End().AddDays(date_shift_.end_days)));
      ++entries_iterator;
    }

    signal_transform_date_entries_(transformed_entries);
  }

  void InputTransformedDateIntervals(
      const std::vector<DateEntry>& date_entries) {
    std::vector<DateEntry> untransformed_entries = date_entries;
    auto entries_iterator = untransformed_entries.begin();

    for (const auto& date_entry : date_entries) {
      const auto& interval = date_entry.GetDateInterval();
      entries_iterator->SetDateInterval(
          DatePeriod(interval.Begin().AddDays(-date_shift_.begin_days),
                     interval.End().AddDays(-date_shift_.end_days)));
      ++entries_iterator;
    }

    signal_date_entries_(untransformed_entries);
  }

  void SetTransform(DateShift shift) { date_shift_ = shift; }

  sigslot::signal<const std::vector<DateEntry>&>& SignalDateEntries() {
    return signal_date_entries_;
  }
  sigslot::signal<const std::vector<DateEntry>&>& SignalTransformDateEntries() {
    return signal_transform_date_entries_;
  }

 private:
  sigslot::signal<const std::vector<DateEntry>&> signal_date_entries_;
  sigslot::signal<const std::vector<DateEntry>&> signal_transform_date_entries_;
  DateShift date_shift_;
  bool emitting_{false};
};
#endif  // TRANSFORM_DATE_ENTRY_HPP
