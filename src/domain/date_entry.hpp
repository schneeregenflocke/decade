#ifndef DATE_ENTRY_HPP
#define DATE_ENTRY_HPP

#include "date_period.hpp"

class DateEntry {
 public:
  DateEntry() = default;  // both periods invalid until set

  [[nodiscard]] const DatePeriod& GetDateInterval() const {
    return date_interval_;
  }
  void SetDateInterval(const DatePeriod& value) { date_interval_ = value; }

  [[nodiscard]] const DatePeriod& GetDateInterInterval() const {
    return date_inter_interval_;
  }
  void SetDateInterInterval(const DatePeriod& value) {
    date_inter_interval_ = value;
  }

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

  [[nodiscard]] int GetGroupNumber() const { return group_number_; }
  void SetGroupNumber(int group_number) { group_number_ = group_number; }

 private:
  DatePeriod date_interval_;
  DatePeriod date_inter_interval_;
  int number_{0};
  int group_{0};
  int group_number_{0};
};
#endif  // DATE_ENTRY_HPP
