#ifndef DATE_ENTRY_HPP
#define DATE_ENTRY_HPP

#include "date_period.hpp"

class DateEntry {
 public:
  DateEntry() = default;  // both periods invalid until set

  [[nodiscard]] const DatePeriod& GetDateInterval() const;
  void SetDateInterval(const DatePeriod& value);

  [[nodiscard]] const DatePeriod& GetDateInterInterval() const;
  void SetDateInterInterval(const DatePeriod& value);

  [[nodiscard]] int GetNumber() const;
  void SetNumber(int number);

  [[nodiscard]] int GetGroup() const;
  void SetGroup(int group);

  [[nodiscard]] int GetGroupNumber() const;
  void SetGroupNumber(int group_number);

 private:
  DatePeriod date_interval_;
  DatePeriod date_inter_interval_;
  int number_{0};
  int group_{0};
  int group_number_{0};
};
#endif  // DATE_ENTRY_HPP
