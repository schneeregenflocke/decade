#ifndef GROUP_STORE_HPP
#define GROUP_STORE_HPP

#include <algorithm>
#include <sigslot/signal.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "detail/reentry_guard.hpp"

// Pure domain value: one date group. No serialization, no signal -> Rule of
// Zero, freely copyable. Persistence is handled non-intrusively in the
// infrastructure layer (see app/services/project_serialization.hpp).
class DateGroup {
 public:
  DateGroup() = default;

  explicit DateGroup(std::string name) : name_(std::move(name)) {}

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] const std::string& GetName() const { return name_; }
  void SetName(std::string name) { name_ = std::move(name); }

  [[nodiscard]] bool IsExcluded() const { return exclude_; }
  void SetExcluded(bool exclude) { exclude_ = exclude; }

 private:
  int number_{0};
  std::string name_{"no name"};
  bool exclude_{false};
};

// Pure value object: the set of date groups plus the queries that are the
// information expert over that data. No signal, no identity -> Rule of Zero,
// freely copyable. The owning DateGroupStore adds the publish/subscribe and
// re-entry concerns on top.
class DateGroups {
 public:
  // Replace the contents and renumber the groups in order.
  void Assign(const std::vector<DateGroup>& incoming_date_groups) {
    date_groups = incoming_date_groups;
    UpdateNumbers();
  }

  [[nodiscard]] const std::vector<DateGroup>& Items() const {
    return date_groups;
  }

  [[nodiscard]] int GetNumber(const std::string& name) const {
    auto find_lambda = [&](const DateGroup& compare) {
      return compare.GetName() == name;
    };
    auto found =
        std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->GetNumber();
    }
    throw std::runtime_error("number not found");
  }

  [[nodiscard]] std::string GetName(int number) const {
    auto find_lambda = [&](const DateGroup& compare) {
      return compare.GetNumber() == number;
    };
    auto found =
        std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->GetName();
    }
    throw std::runtime_error("string not found");
  }

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const {
    std::vector<std::string> name_strings(date_groups.size());
    auto iterator = name_strings.begin();
    for (const auto& date_group : date_groups) {
      *iterator = date_group.GetName();
      ++iterator;
    }
    return name_strings;
  }

  [[nodiscard]] int GetGroupMax() const {
    if (date_groups.empty()) {
      return -1;
    }
    return static_cast<int>(date_groups.size()) - 1;
  }

  [[nodiscard]] bool GetExclude(int number) const {
    return date_groups.at(static_cast<size_t>(number)).IsExcluded();
  }

 private:
  void UpdateNumbers() {
    int number = 0;
    for (auto& date_group : date_groups) {
      date_group.SetNumber(number);
      ++number;
    }
  }
  std::vector<DateGroup> date_groups;
};

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
    date_groups.Assign(incoming_date_groups);
    signal_date_groups(date_groups.Items());
  }

  [[nodiscard]] const std::vector<DateGroup>& GetDateGroups() const {
    return date_groups.Items();
  }

  [[nodiscard]] const DateGroups& Value() const { return date_groups; }

  // call after connecting
  void SendDefaultValues() {
    std::vector<DateGroup> temporary_date_groups;
    temporary_date_groups.emplace_back("Default");
    ReceiveDateGroups(temporary_date_groups);
  }

  [[nodiscard]] int GetNumber(const std::string& name) const {
    return date_groups.GetNumber(name);
  }

  [[nodiscard]] std::string GetName(int number) const {
    return date_groups.GetName(number);
  }

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const {
    return date_groups.GetDateGroupsNames();
  }

  [[nodiscard]] int GetGroupMax() const { return date_groups.GetGroupMax(); }

  [[nodiscard]] bool GetExclude(int number) const {
    return date_groups.GetExclude(number);
  }

  sigslot::signal<const std::vector<DateGroup>&>& SignalDateGroups() {
    return signal_date_groups;
  }

 private:
  DateGroups date_groups;
  sigslot::signal<const std::vector<DateGroup>&> signal_date_groups;
  bool emitting_{false};
};
#endif  // GROUP_STORE_HPP
