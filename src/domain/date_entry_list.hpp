#ifndef DATE_ENTRY_LIST_HPP
#define DATE_ENTRY_LIST_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <vector>

#include "date_entry.hpp"
#include "date_group.hpp"
#include "date_period.hpp"

// A value object: the entries of a project in canonical form. `Assign` discards
// null periods, sorts by begin and derives everything derived anew — the
// running number, the gap period to the next entry, the number within the group
// — and cuts groups that no longer exist.
//
// Separate from the store, because two holders need the same preparation but
// only one of them publishes: DateEntryStore publishes, DateEntryBarStore
// computes bars out of it. They used to share this through inheritance with
// protected access; composition manages without both.
class DateEntryList {
 public:
  void Assign(const std::vector<DateEntry>& incoming_date_entries) {
    date_entries_.clear();
    date_entries_.reserve(incoming_date_entries.size());

    // Null periods carry no day and are dropped; everything stored is a
    // well-formed half-open interval.
    for (const auto& date_entry : incoming_date_entries) {
      if (!date_entry.GetDateInterval().IsNull()) {
        date_entries_.push_back(date_entry);
      }
    }

    date_entries_.shrink_to_fit();

    Sort();
    ClampGroupsToKnownRange();
    AssignNumbers();
    AssignInterIntervals();
    AssignGroupNumbers();
  }

  void AssignDateGroups(const std::vector<DateGroup>& incoming_date_groups) {
    date_groups_.Assign(incoming_date_groups);
  }

  [[nodiscard]] const std::vector<DateEntry>& Items() const {
    return date_entries_;
  }

  [[nodiscard]] bool IsEmpty() const { return date_entries_.empty(); }

  [[nodiscard]] std::size_t YearSpan() const {
    if (date_entries_.empty()) {
      return 0;
    }
    const int year_span = LastYear() - FirstYear() + 1;
    return static_cast<std::size_t>(year_span);
  }

  [[nodiscard]] int FirstYear() const {
    if (date_entries_.empty()) {
      return 0;
    }
    return date_entries_.front().GetDateInterval().Begin().Year();
  }

  [[nodiscard]] int LastYear() const {
    if (date_entries_.empty()) {
      return 0;
    }
    // The sort is by Begin(); the latest End() can sit on an earlier-beginning,
    // multi-year entry — so maximise across every entry.
    const auto& latest = std::ranges::max(
        date_entries_, {},
        [](const DateEntry& entry) { return entry.GetDateInterval().Last(); });
    return latest.GetDateInterval().Last().Year();
  }

 private:
  void Sort() {
    std::ranges::sort(date_entries_, {}, [](const DateEntry& entry) {
      return entry.GetDateInterval().Begin();
    });
  }

  void AssignNumbers() {
    int current_number = 0;
    for (auto& date_entry : date_entries_) {
      date_entry.SetNumber(current_number);
      ++current_number;
    }
  }

  void AssignInterIntervals() {
    for (auto iterator = date_entries_.begin(); iterator != date_entries_.end();
         ++iterator) {
      const auto next = iterator + 1;
      if (next != date_entries_.end()) {
        iterator->SetDateInterInterval(
            DatePeriod(iterator->GetDateInterval().End(),
                       next->GetDateInterval().Begin()));
      }
    }
  }

  void AssignGroupNumbers() {
    std::map<int, int> groups_counter;

    for (auto& date_entry : date_entries_) {
      const int current_group = date_entry.GetGroup();

      if (groups_counter.contains(current_group)) {
        groups_counter[current_group] += 1;
      } else {
        groups_counter[current_group] = 0;
      }

      date_entry.SetGroupNumber(groups_counter[current_group]);
    }
  }

  void ClampGroupsToKnownRange() {
    for (auto& date_entry : date_entries_) {
      if (date_entry.GetGroup() > date_groups_.GetGroupMax()) {
        date_entry.SetGroup(0);
      }
    }
  }

  std::vector<DateEntry> date_entries_;
  DateGroups date_groups_;
};

#endif  // DATE_ENTRY_LIST_HPP
