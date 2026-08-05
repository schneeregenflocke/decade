#ifndef DATE_GROUP_STORE_HPP
#define DATE_GROUP_STORE_HPP

#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

// Owns a DateGroups value and publishes every change on the injected topic. It
// has identity -> not copyable. It carries no serialisation code.
class DateGroupStore {
 public:
  explicit DateGroupStore(domain::StateTopic<std::vector<DateGroup>>& topic)
      : topic_(topic) {}
  ~DateGroupStore() = default;
  DateGroupStore(const DateGroupStore&) = delete;
  DateGroupStore& operator=(const DateGroupStore&) = delete;
  DateGroupStore(DateGroupStore&&) = delete;
  DateGroupStore& operator=(DateGroupStore&&) = delete;

  void ReceiveDateGroups(const std::vector<DateGroup>& incoming_date_groups) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    date_groups_.Assign(incoming_date_groups);
    topic_(date_groups_.Items());
  }

  [[nodiscard]] const DateGroups& Get() const { return date_groups_; }

  // Call after wiring: it sets the one default group and publishes it, so every
  // consumer holds an initial value.
  void SendDefaultValues() {
    std::vector<DateGroup> temporary_date_groups;
    temporary_date_groups.emplace_back("Default");
    ReceiveDateGroups(temporary_date_groups);
  }

 private:
  DateGroups date_groups_;
  domain::StateTopic<std::vector<DateGroup>>& topic_;
  bool emitting_{false};
};
#endif  // DATE_GROUP_STORE_HPP
