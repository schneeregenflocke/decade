#ifndef DATE_INTERVAL_BUNDLE_HPP
#define DATE_INTERVAL_BUNDLE_HPP

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/special_defs.hpp>
#include <string>
#include <utility>

class DateIntervalBundle {
 public:
  DateIntervalBundle()
      : date_interval_(boost::gregorian::date_period(
            boost::gregorian::date(boost::date_time::not_a_date_time),
            boost::gregorian::date(boost::date_time::not_a_date_time))),
        date_inter_interval_(boost::gregorian::date_period(
            boost::gregorian::date(boost::date_time::not_a_date_time),
            boost::gregorian::date(boost::date_time::not_a_date_time))) {}

  [[nodiscard]] const boost::gregorian::date_period& GetDateInterval() const {
    return date_interval_;
  }
  boost::gregorian::date_period& MutableDateInterval() {
    return date_interval_;
  }
  void SetDateInterval(const boost::gregorian::date_period& value) {
    date_interval_ = value;
  }

  [[nodiscard]] const boost::gregorian::date_period& GetDateInterInterval()
      const {
    return date_inter_interval_;
  }
  void SetDateInterInterval(const boost::gregorian::date_period& value) {
    date_inter_interval_ = value;
  }

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] int GetGroup() const { return group_; }
  void SetGroup(int group) { group_ = group; }

  [[nodiscard]] int GetGroupNumber() const { return group_number_; }
  void SetGroupNumber(int group_number) { group_number_ = group_number; }

  [[nodiscard]] bool IsExcluded() const { return exclude_; }
  void SetExcluded(bool exclude) { exclude_ = exclude; }

  [[nodiscard]] const std::string& GetComment() const { return comment_; }
  void SetComment(std::string comment) { comment_ = std::move(comment); }

 private:
  boost::gregorian::date_period date_interval_;
  boost::gregorian::date_period date_inter_interval_;
  int number_{0};
  int group_{0};
  int group_number_{0};
  std::string comment_;
  bool exclude_{false};
};
#endif  // DATE_INTERVAL_BUNDLE_HPP
