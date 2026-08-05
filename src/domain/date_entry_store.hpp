#ifndef DATE_ENTRY_STORE_HPP
#define DATE_ENTRY_STORE_HPP

#include <vector>

#include "date_entry.hpp"
#include "date_entry_list.hpp"
#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

// Owns the entries of a project and publishes every change on the injected
// topic. It has identity -> not copyable.
class DateEntryStore {
 public:
  explicit DateEntryStore(domain::StateTopic<std::vector<DateEntry>>& topic)
      : topic_(topic) {}
  ~DateEntryStore() = default;
  DateEntryStore(const DateEntryStore&) = delete;
  DateEntryStore& operator=(const DateEntryStore&) = delete;
  DateEntryStore(DateEntryStore&&) = delete;
  DateEntryStore& operator=(DateEntryStore&&) = delete;

  void ReceiveDateEntries(const std::vector<DateEntry>& incoming_date_entries) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    date_entries_.Assign(incoming_date_entries);
    topic_(date_entries_.Items());
  }

  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    date_entries_.AssignDateGroups(date_groups);
  }

  [[nodiscard]] const DateEntryList& Get() const { return date_entries_; }

 private:
  DateEntryList date_entries_;
  domain::StateTopic<std::vector<DateEntry>>& topic_;
  bool emitting_{false};
};
#endif  // DATE_ENTRY_STORE_HPP
