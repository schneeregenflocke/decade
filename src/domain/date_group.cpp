#include "date_group.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

DateGroup::DateGroup(std::string name) : name_(std::move(name)) {}

int DateGroup::GetNumber() const { return number_; }

void DateGroup::SetNumber(int number) { number_ = number; }

const std::string& DateGroup::GetName() const { return name_; }

void DateGroup::SetName(std::string name) { name_ = std::move(name); }

void DateGroups::Assign(const std::vector<DateGroup>& incoming_date_groups) {
  date_groups_ = incoming_date_groups;
  UpdateNumbers();
}

const std::vector<DateGroup>& DateGroups::Items() const { return date_groups_; }

int DateGroups::GetNumber(const std::string& name) const {
  auto find_lambda = [&](const DateGroup& compare) {
    return compare.GetName() == name;
  };
  auto found = std::ranges::find_if(date_groups_, find_lambda);
  if (found != date_groups_.end()) {
    return found->GetNumber();
  }
  throw std::runtime_error("number not found");
}

std::string DateGroups::GetName(int number) const {
  auto find_lambda = [&](const DateGroup& compare) {
    return compare.GetNumber() == number;
  };
  auto found = std::ranges::find_if(date_groups_, find_lambda);
  if (found != date_groups_.end()) {
    return found->GetName();
  }
  throw std::runtime_error("string not found");
}

std::vector<std::string> DateGroups::GetDateGroupsNames() const {
  std::vector<std::string> name_strings;
  name_strings.reserve(date_groups_.size());
  std::ranges::transform(date_groups_, std::back_inserter(name_strings),
                         &DateGroup::GetName);
  return name_strings;
}

int DateGroups::GetGroupMax() const {
  if (date_groups_.empty()) {
    return -1;
  }
  return static_cast<int>(date_groups_.size()) - 1;
}

void DateGroups::UpdateNumbers() {
  int number = 0;
  for (auto& date_group : date_groups_) {
    date_group.SetNumber(number);
    ++number;
  }
}
