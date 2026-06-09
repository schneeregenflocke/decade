#ifndef DATE_GROUP_HPP
#define DATE_GROUP_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Pure domain value: one date group. No serialization, no signal -> Rule of
// Zero, freely copyable. Persistence is handled non-intrusively in the
// infrastructure layer (see app/services/value_serialization.hpp).
class DateGroup {
 public:
  DateGroup() = default;

  explicit DateGroup(std::string name) : name_(std::move(name)) {}

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] const std::string& GetName() const { return name_; }
  void SetName(std::string name) { name_ = std::move(name); }

 private:
  int number_{0};
  std::string name_{"no name"};
};

// Pure value object: the set of date groups plus the queries that are the
// information expert over that data. No signal, no identity -> Rule of Zero,
// freely copyable. The owning DateGroupStore adds the publish/subscribe and
// re-entry concerns on top.
class DateGroups {
 public:
  // Replace the contents and renumber the groups in order.
  void Assign(const std::vector<DateGroup>& incoming_date_groups) {
    date_groups_ = incoming_date_groups;
    UpdateNumbers();
  }

  [[nodiscard]] const std::vector<DateGroup>& Items() const {
    return date_groups_;
  }

  [[nodiscard]] int GetNumber(const std::string& name) const {
    auto find_lambda = [&](const DateGroup& compare) {
      return compare.GetName() == name;
    };
    auto found = std::ranges::find_if(date_groups_, find_lambda);
    if (found != date_groups_.end()) {
      return found->GetNumber();
    }
    throw std::runtime_error("number not found");
  }

  [[nodiscard]] std::string GetName(int number) const {
    auto find_lambda = [&](const DateGroup& compare) {
      return compare.GetNumber() == number;
    };
    auto found = std::ranges::find_if(date_groups_, find_lambda);
    if (found != date_groups_.end()) {
      return found->GetName();
    }
    throw std::runtime_error("string not found");
  }

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const {
    std::vector<std::string> name_strings(date_groups_.size());
    auto iterator = name_strings.begin();
    for (const auto& date_group : date_groups_) {
      *iterator = date_group.GetName();
      ++iterator;
    }
    return name_strings;
  }

  [[nodiscard]] int GetGroupMax() const {
    if (date_groups_.empty()) {
      return -1;
    }
    return static_cast<int>(date_groups_.size()) - 1;
  }

 private:
  void UpdateNumbers() {
    int number = 0;
    for (auto& date_group : date_groups_) {
      date_group.SetNumber(number);
      ++number;
    }
  }
  std::vector<DateGroup> date_groups_;
};
#endif  // DATE_GROUP_HPP
