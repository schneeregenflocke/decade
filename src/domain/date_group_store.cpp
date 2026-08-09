#include "date_group_store.hpp"

#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

DateGroupStore::DateGroupStore(
    domain::StateTopic<std::vector<DateGroup>>& topic)
    : topic_(topic) {}

void DateGroupStore::ReceiveDateGroups(
    const std::vector<DateGroup>& incoming_date_groups) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  date_groups_.Assign(incoming_date_groups);
  topic_(date_groups_.Items());
}

const DateGroups& DateGroupStore::Get() const { return date_groups_; }

void DateGroupStore::SendDefaultValues() {
  std::vector<DateGroup> temporary_date_groups;
  temporary_date_groups.emplace_back("Default");
  ReceiveDateGroups(temporary_date_groups);
}
