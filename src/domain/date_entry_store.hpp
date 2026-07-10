#ifndef DATE_ENTRY_STORE_HPP
#define DATE_ENTRY_STORE_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <sigslot/signal.hpp>
#include <vector>

#include "date_entry.hpp"
#include "date_group.hpp"
#include "date_group_store.hpp"
#include "date_period.hpp"
#include "detail/reentry_guard.hpp"

class DateEntryStore {
 public:
  DateEntryStore() = default;
  virtual ~DateEntryStore() = default;
  DateEntryStore(const DateEntryStore&) = delete;
  DateEntryStore& operator=(const DateEntryStore&) = delete;
  DateEntryStore(DateEntryStore&&) = delete;
  DateEntryStore& operator=(DateEntryStore&&) = delete;

  virtual void ReceiveDateEntries(
      const std::vector<DateEntry>& incoming_date_entries) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    ProcessDateEntries(incoming_date_entries);
    signal_date_entries_(date_entries_);
  }

  [[nodiscard]] const std::vector<DateEntry>& GetDateEntries() const {
    return date_entries_;
  }

  [[nodiscard]] bool is_empty() const { return date_entries_.empty(); }

  [[nodiscard]] std::size_t GetSpan() const {
    if (date_entries_.empty()) {
      return 0;
    }
    const int year_span = GetLastYear() - GetFirstYear() + 1;
    return static_cast<std::size_t>(year_span);
  }

  [[nodiscard]] int GetFirstYear() const {
    if (date_entries_.empty()) {
      return 0;
    }
    return date_entries_.front().GetDateInterval().Begin().Year();
  }

  [[nodiscard]] int GetLastYear() const {
    if (date_entries_.empty()) {
      return 0;
    }
    // Stored periods are never null (filtered in ProcessDateEntries), so
    // Last() >= Begin() always holds.
    return date_entries_.back().GetDateInterval().Last().Year();
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_group_store_.ReceiveDateGroups(date_groups);
  }

  sigslot::signal<const std::vector<DateEntry>&>& SignalDateEntries() {
    return signal_date_entries_;
  }

 protected:
  // Re-entry guard flag, owned privately but reachable by subclasses through
  // this accessor (the flag itself stays private to keep data encapsulated).
  [[nodiscard]] bool& Emitting() { return emitting_; }

  [[nodiscard]] const std::vector<DateEntry>& GetDateEntriesInternal() const {
    return date_entries_;
  }
  std::vector<DateEntry>& MutableDateEntries() { return date_entries_; }

  [[nodiscard]] const DateGroupStore& GetDateGroupStore() const {
    return date_group_store_;
  }
  DateGroupStore& MutableDateGroupStore() { return date_group_store_; }

  void ProcessDateEntries(const std::vector<DateEntry>& incoming_date_entries) {
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
    CheckAndAdjustGroupIntegrity();
    ProcessNumbers();
    ProcessDateInterIntervals();
    ProcessDateGroupsNumber();
  }

 private:
  bool emitting_{false};
  std::vector<DateEntry> date_entries_;
  DateGroupStore date_group_store_;

  sigslot::signal<const std::vector<DateEntry>&> signal_date_entries_;

  void Sort() {
    auto sort_func = [](const DateEntry& entry0, const DateEntry& entry1) {
      return entry0.GetDateInterval().Begin() <
             entry1.GetDateInterval().Begin();
    };

    std::ranges::sort(date_entries_, sort_func);
  }

  void ProcessNumbers() {
    int current_number = 0;
    for (auto& date_entry : date_entries_) {
      date_entry.SetNumber(current_number);
      ++current_number;
    }
  }

  void ProcessDateInterIntervals() {
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

  void ProcessDateGroupsNumber() {
    std::map<int, int> groups_counter;

    for (auto& date_entry : date_entries_) {
      auto current_group = date_entry.GetGroup();

      if (groups_counter.contains(current_group)) {
        groups_counter[current_group] += 1;
      } else {
        groups_counter[current_group] = 0;
      }

      date_entry.SetGroupNumber(groups_counter[current_group]);
    }
  }

  void CheckAndAdjustGroupIntegrity() {
    for (auto& date_entry : date_entries_) {
      if (date_entry.GetGroup() > date_group_store_.GetGroupMax()) {
        date_entry.SetGroup(0);
      }
    }
  }
};
#endif  // DATE_ENTRY_STORE_HPP
