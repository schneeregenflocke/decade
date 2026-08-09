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

  explicit TransformDateEntry(
      domain::StateTopic<std::vector<DateEntry>>& topic);

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries);

  void SetTransform(DateShift shift);

 private:
  domain::StateTopic<std::vector<DateEntry>>& topic_;
  DateShift date_shift_;
  bool emitting_{false};
};
#endif  // TRANSFORM_DATE_ENTRY_HPP
