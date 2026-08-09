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

int DateEntry::GetGroup() const { return group_; }

void DateEntry::SetGroup(int group) { group_ = group; }

int DateEntry::GetGroupNumber() const { return group_number_; }

void DateEntry::SetGroupNumber(int group_number) {
  group_number_ = group_number;
}
