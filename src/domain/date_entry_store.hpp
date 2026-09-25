#ifndef DATE_ENTRY_STORE_HPP
#define DATE_ENTRY_STORE_HPP

#include <vector>

#include "date_category.hpp"
#include "date_entry.hpp"
#include "date_entry_list.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

// Owns the entries of a project and publishes every change on the injected
// topic. It has identity -> not copyable.
class DateEntryStore {
 public:
  explicit DateEntryStore(domain::DateEntriesTopic& topic);
  ~DateEntryStore() = default;
  DateEntryStore(const DateEntryStore&) = delete;
  DateEntryStore& operator=(const DateEntryStore&) = delete;
  DateEntryStore(DateEntryStore&&) = delete;
  DateEntryStore& operator=(DateEntryStore&&) = delete;

  void ReceiveDateEntries(const std::vector<DateEntry>& incoming_date_entries);

  void ReceiveDateCategories(const std::vector<DateCategory>& date_categories);

  [[nodiscard]] const DateEntryList& Get() const;

 private:
  DateEntryList date_entries_;
  domain::DateEntriesTopic& topic_;
  bool emitting_{false};
};
#endif  // DATE_ENTRY_STORE_HPP
