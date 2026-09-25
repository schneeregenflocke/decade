#include "date_entry_store.hpp"

#include <vector>

#include "date_category.hpp"
#include "date_entry.hpp"
#include "date_entry_list.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

DateEntryStore::DateEntryStore(domain::DateEntriesTopic& topic)
    : topic_(topic) {}

void DateEntryStore::ReceiveDateEntries(
    const std::vector<DateEntry>& incoming_date_entries) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  date_entries_.Assign(incoming_date_entries);
  topic_.Publish(date_entries_.Items());
}

void DateEntryStore::ReceiveDateCategories(
    const std::vector<DateCategory>& date_categories) {
  date_entries_.AssignDateCategories(date_categories);
}

const DateEntryList& DateEntryStore::Get() const { return date_entries_; }
