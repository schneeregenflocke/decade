#include "transform_date_entry.hpp"

#include <vector>

#include "date_entry.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

TransformDateEntry::TransformDateEntry(
    domain::StateTopic<std::vector<DateEntry>>& topic)
    : topic_(topic), date_shift_{.begin_days = 0, .end_days = 0} {}

void TransformDateEntry::ReceiveDateEntries(
    const std::vector<DateEntry>& date_entries) {
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

void TransformDateEntry::SetTransform(DateShift shift) { date_shift_ = shift; }
