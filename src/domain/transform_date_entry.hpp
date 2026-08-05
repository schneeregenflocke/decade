#ifndef TRANSFORM_DATE_ENTRY_HPP
#define TRANSFORM_DATE_ENTRY_HPP

#include <vector>

#include "date_entry.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

// Shifts the begin and end of every entry by a fixed number of days and
// publishes the result on the injected topic. The shift is zero everywhere
// today; the path stays because it separates the display from the stored data.
class TransformDateEntry {
 public:
  struct DateShift {
    int begin_days;
    int end_days;
  };

  explicit TransformDateEntry(domain::StateTopic<std::vector<DateEntry>>& topic)
      : topic_(topic), date_shift_{.begin_days = 0, .end_days = 0} {}

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    std::vector<DateEntry> transformed_entries = date_entries;

    for (auto& transformed_entry : transformed_entries) {
      const auto& interval = transformed_entry.GetDateInterval();
      transformed_entry.SetDateInterval(
          DatePeriod(interval.Begin().AddDays(date_shift_.begin_days),
                     interval.End().AddDays(date_shift_.end_days)));
    }

    topic_(transformed_entries);
  }

  void SetTransform(DateShift shift) { date_shift_ = shift; }

 private:
  domain::StateTopic<std::vector<DateEntry>>& topic_;
  DateShift date_shift_;
  bool emitting_{false};
};
#endif  // TRANSFORM_DATE_ENTRY_HPP
