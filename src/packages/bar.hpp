#ifndef BAR_HPP
#define BAR_HPP

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cstdint>
#include <string>

class Bar {
 public:
  explicit Bar(const boost::gregorian::date_period& date_interval)
      : date_interval_(date_interval) {}

  void SetText(const std::string& text) { text_ = text; }

  [[nodiscard]] const std::string& GetText() const { return text_; }

  [[nodiscard]] int GetYear() const { return date_interval_.begin().year(); }

  [[nodiscard]] std::int64_t GetLenght() const {
    return date_interval_.length().days();
  }

  [[nodiscard]] float GetFirstDay() const {
    return static_cast<float>(date_interval_.begin().day_of_year() - 1);
  }

  [[nodiscard]] float GetLastDay() const {
    return static_cast<float>(date_interval_.last().day_of_year());
  }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

 private:
  boost::gregorian::date_period date_interval_;
  std::string text_;
  int group_{0};
};
#endif  // BAR_HPP
