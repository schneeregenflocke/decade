#ifndef BAR_HPP
#define BAR_HPP

#include <cstdint>
#include <string>

#include "date_period.hpp"

class Bar {
 public:
  explicit Bar(const DatePeriod& date_interval);

  void SetText(const std::string& text);

  [[nodiscard]] const std::string& GetText() const;

  [[nodiscard]] int GetYear() const;

  [[nodiscard]] std::int64_t GetLength() const;

  [[nodiscard]] float GetFirstDay() const;

  [[nodiscard]] float GetLastDay() const;

  [[nodiscard]] int GetCategory() const;
  void SetCategory(int category);

 private:
  DatePeriod date_interval_;
  std::string text_;
  int category_{0};
};
#endif  // BAR_HPP
