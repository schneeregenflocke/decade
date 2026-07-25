#ifndef TRANSFORM_DATE_ENTRY_HPP
#define TRANSFORM_DATE_ENTRY_HPP

#include <vector>

#include "date_entry.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

// Verschiebt Beginn und Ende jedes Eintrags um eine feste Tageszahl und
// veröffentlicht das Ergebnis auf dem eingesetzten Topic. Die Verschiebung ist
// heute überall null; der Weg bleibt, weil er die Darstellung von den
// gespeicherten Daten trennt.
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
