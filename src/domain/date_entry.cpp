#include "date_entry.hpp"

#include "date_period.hpp"

const DatePeriod& DateEntry::GetDateInterval() const { return date_interval_; }

void DateEntry::SetDateInterval(const DatePeriod& value) {
  date_interval_ = value;
}

const DatePeriod& DateEntry::GetDateInterInterval() const {
  return date_inter_interval_;
}

void DateEntry::SetDateInterInterval(const DatePeriod& value) {
  date_inter_interval_ = value;
}

int DateEntry::GetNumber() const { return number_; }

void DateEntry::SetNumber(int number) { number_ = number; }

int DateEntry::GetCategory() const { return category_; }

void DateEntry::SetCategory(int category) { category_ = category; }

int DateEntry::GetCategoryNumber() const { return category_number_; }

void DateEntry::SetCategoryNumber(int category_number) {
  category_number_ = category_number;
}
