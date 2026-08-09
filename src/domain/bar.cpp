#include "bar.hpp"

#include <cstdint>
#include <string>

#include "date_period.hpp"

Bar::Bar(const DatePeriod& date_interval) : date_interval_(date_interval) {}

void Bar::SetText(const std::string& text) { text_ = text; }

const std::string& Bar::GetText() const { return text_; }

int Bar::GetYear() const { return date_interval_.Begin().Year(); }

std::int64_t Bar::GetLength() const { return date_interval_.LengthDays(); }

float Bar::GetFirstDay() const {
  return static_cast<float>(date_interval_.Begin().DayOfYear() - 1);
}

float Bar::GetLastDay() const {
  return static_cast<float>(date_interval_.Last().DayOfYear());
}

int Bar::GetGroup() const { return group_; }

void Bar::SetGroup(int group) { group_ = group; }
