#include "date_category.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

DateCategory::DateCategory(std::string name) : name_(std::move(name)) {}

int DateCategory::GetNumber() const { return number_; }

void DateCategory::SetNumber(int number) { number_ = number; }

const std::string& DateCategory::GetName() const { return name_; }

void DateCategory::SetName(std::string name) { name_ = std::move(name); }

void DateCategories::Assign(
    const std::vector<DateCategory>& incoming_date_categories) {
  date_categories_ = incoming_date_categories;
  UpdateNumbers();
}

const std::vector<DateCategory>& DateCategories::Items() const {
  return date_categories_;
}

int DateCategories::GetNumber(const std::string& name) const {
  auto find_lambda = [&](const DateCategory& compare) {
    return compare.GetName() == name;
  };
  auto found = std::ranges::find_if(date_categories_, find_lambda);
  if (found != date_categories_.end()) {
    return found->GetNumber();
  }
  throw std::runtime_error("number not found");
}

std::string DateCategories::GetName(int number) const {
  auto find_lambda = [&](const DateCategory& compare) {
    return compare.GetNumber() == number;
  };
  auto found = std::ranges::find_if(date_categories_, find_lambda);
  if (found != date_categories_.end()) {
    return found->GetName();
  }
  throw std::runtime_error("string not found");
}

std::vector<std::string> DateCategories::GetDateCategoryNames() const {
  std::vector<std::string> name_strings;
  name_strings.reserve(date_categories_.size());
  std::ranges::transform(date_categories_, std::back_inserter(name_strings),
                         &DateCategory::GetName);
  return name_strings;
}

int DateCategories::GetCategoryMax() const {
  if (date_categories_.empty()) {
    return -1;
  }
  return static_cast<int>(date_categories_.size()) - 1;
}

void DateCategories::UpdateNumbers() {
  int number = 0;
  for (auto& date_category : date_categories_) {
    date_category.SetNumber(number);
    ++number;
  }
}
