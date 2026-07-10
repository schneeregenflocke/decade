#ifndef BAR_HPP
#define BAR_HPP

#include <cstdint>
#include <string>

#include "date_period.hpp"

class Bar {
 public:
  explicit Bar(const DatePeriod& date_interval)
      : date_interval_(date_interval) {}

  void SetText(const std::string& text) { text_ = text; }

  [[nodiscard]] const std::string& GetText() const { return text_; }

  [[nodiscard]] int GetYear() const { return date_interval_.Begin().Year(); }

  [[nodiscard]] std::int64_t GetLength() const {
    return date_interval_.LengthDays();
  }

  [[nodiscard]] float GetFirstDay() const {
    return static_cast<float>(date_interval_.Begin().DayOfYear() - 1);
  }

  [[nodiscard]] float GetLastDay() const {
    return static_cast<float>(date_interval_.Last().DayOfYear());
  }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

 private:
  DatePeriod date_interval_;
  std::string text_;
  int group_{0};
};
#endif  // BAR_HPP
