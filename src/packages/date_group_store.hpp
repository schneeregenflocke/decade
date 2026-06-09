#ifndef DATE_GROUP_STORE_HPP
#define DATE_GROUP_STORE_HPP

#include <sigslot/signal.hpp>
#include <string>
#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"

// Owns a DateGroups value plus the change signal and the re-entry guard. Has
// identity -> non-copyable. Delegates all queries to the value; the public API
// is unchanged so callers stay stable. Carries no serialization code.
class DateGroupStore {
 public:
  DateGroupStore() = default;
  ~DateGroupStore() = default;
  DateGroupStore(const DateGroupStore&) = delete;
  DateGroupStore& operator=(const DateGroupStore&) = delete;
  DateGroupStore(DateGroupStore&&) = delete;
  DateGroupStore& operator=(DateGroupStore&&) = delete;

  void ReceiveDateGroups(const std::vector<DateGroup>& incoming_date_groups) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    date_groups_.Assign(incoming_date_groups);
    signal_date_groups_(date_groups_.Items());
  }

  [[nodiscard]] const std::vector<DateGroup>& GetDateGroups() const {
    return date_groups_.Items();
  }

  [[nodiscard]] const DateGroups& Value() const { return date_groups_; }

  // call after connecting
  void SendDefaultValues() {
    std::vector<DateGroup> temporary_date_groups;
    temporary_date_groups.emplace_back("Default");
    ReceiveDateGroups(temporary_date_groups);
  }

  [[nodiscard]] int GetNumber(const std::string& name) const {
    return date_groups_.GetNumber(name);
  }

  [[nodiscard]] std::string GetName(int number) const {
    return date_groups_.GetName(number);
  }

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const {
    return date_groups_.GetDateGroupsNames();
  }

  [[nodiscard]] int GetGroupMax() const { return date_groups_.GetGroupMax(); }

  sigslot::signal<const std::vector<DateGroup>&>& SignalDateGroups() {
    return signal_date_groups_;
  }

 private:
  DateGroups date_groups_;
  sigslot::signal<const std::vector<DateGroup>&> signal_date_groups_;
  bool emitting_{false};
};
#endif  // DATE_GROUP_STORE_HPP
