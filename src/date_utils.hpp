#ifndef HOME_TITAN99_CODE_DECADE_SRC_DATE_UTILS_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_DATE_UTILS_HPP

#include <boost/date_time/gregorian/conversion.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/special_defs.hpp>

#include <array>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>


struct date_format_descriptor {
  std::array<size_t, 3> date_order;
  std::array<unsigned char, 3> delimiters;
  bool shortyear;
};

inline date_format_descriptor InitDateFormat()
{
  constexpr int testyear = 1999;
  constexpr int testyearshort = 99;
  constexpr int testmonth = 4;
  constexpr int testday = 23;

  const std::tm test_tm =
      boost::gregorian::to_tm(boost::gregorian::date(testyear, testmonth, testday));
  std::ostringstream ostrm{};
  ostrm << std::put_time(&test_tm, "%x");
  std::string date_string = ostrm.str();

  date_format_descriptor date_format{};

  size_t delimPos = 0;
  for (size_t index = 0; index < 3; ++index) {
    date_string = date_string.substr(delimPos);
    const int number = std::stoi(date_string, &delimPos);
    if (delimPos < date_string.size()) {
      date_format.delimiters.at(index) =
          static_cast<unsigned char>(date_string.at(delimPos));
      ++delimPos;
    } else {
      date_format.delimiters.at(index) = 0;
    }

    if (number == testyear) {
      date_format.date_order.at(index) = 0;
      date_format.shortyear = false;
    }

    if (number == testyearshort) {
      date_format.date_order.at(index) = 0;
      date_format.shortyear = true;
    }

    if (number == testmonth) {
      date_format.date_order.at(index) = 1;
    }

    if (number == testday) {
      date_format.date_order.at(index) = 2;
    }
  }

  return date_format;
}

inline std::string boost_date_to_string(const boost::gregorian::date &date_variable)
{
  std::ostringstream ostrm{};

  if (!date_variable.is_special()) {
    const std::tm datetm = boost::gregorian::to_tm(date_variable);
    ostrm << std::put_time(&datetm, "%x");
  } else {
    ostrm << "invalid date";
  }

  return ostrm.str();
}
inline boost::gregorian::date string_to_boost_date(const std::string &date_string,
                                                   const date_format_descriptor &format)
{
  std::string non_const_date_string = date_string;

  bool failed = false;
  std::array<int, 3> date_parts{};
  size_t delimPos = 0;
  boost::gregorian::date date_variable;

  for (size_t index = 0; index < 3; ++index) {
    if (delimPos >= non_const_date_string.length() || non_const_date_string.empty()) {
      failed = true;
      break;
    }

    non_const_date_string = non_const_date_string.substr(delimPos);
    if (non_const_date_string.empty()) {
      failed = true;
      break;
    }

    if (std::isdigit(static_cast<unsigned char>(non_const_date_string[0])) == 0) {
      failed = true;
      break;
    }

    int number = 0;

    try {
      number = std::stoi(non_const_date_string, &delimPos);
    } catch (const std::exception &) {
      failed = true;
      break;
    }

    ++delimPos;

    if (format.date_order.at(index) == 0 && format.shortyear) {
      constexpr int shortyear_pivot = 50;
      constexpr int shortyear_upper = 100;
      constexpr int shortyear_base_2000 = 2000;
      constexpr int shortyear_base_1900 = 1900;

      if (number >= 0 && number < shortyear_pivot) {
        number += shortyear_base_2000;
      }

      if (number >= shortyear_pivot && number < shortyear_upper) {
        number += shortyear_base_1900;
      }
    }

    date_parts.at(format.date_order.at(index)) = number;
  }

  if (failed) {
    date_variable = boost::gregorian::date(boost::date_time::not_a_date_time);
  } else {
    try {
      date_variable = boost::gregorian::date(date_parts[0], date_parts[1], date_parts[2]);
    } catch (const std::exception &) {
      date_variable = boost::gregorian::date(boost::date_time::not_a_date_time);
    }
  }

  return date_variable;
}

/// case_id 0: invalid date_interval
/// case_id 1: valid date_interval (both dates of the interval are valid, second date greater than
/// first date) case_id 2: valid single date (both dates of the interval are valid and equal)
/// case_id 3: valid single date (first date of the interval is valid)
inline int CheckDateInterval(const boost::gregorian::date &begin_date,
                             const boost::gregorian::date &end_date)
{
  int case_id = 0;

  if (!begin_date.is_special() && !end_date.is_special()) {
    const boost::gregorian::date_period period =
        boost::gregorian::date_period(begin_date, end_date);

    if (!period.is_null()) {
      case_id = 1;
    } else if (begin_date == end_date) {
      case_id = 2;
    }
  } else if (!begin_date.is_special()) {
    case_id = 3;
  }

  return case_id;
}
/// case_id 0: invalid date_interval
/// case_id 1: valid date_interval (both dates of the interval are valid, second date greater than
/// first date) case_id 2: valid single date (both dates of the interval are valid and equal)
/// case_id 3: valid single date, adjust second date to first date (first date of the interval is
/// valid)
inline int CheckAndAdjustDateInterval(boost::gregorian::date_period *date_interval)
{
  int case_id = 0;

  if (!date_interval->begin().is_special() && !date_interval->end().is_special()) {
    if (!date_interval->is_null()) {
      case_id = 1;
    } else if (date_interval->begin() == date_interval->end()) {
      case_id = 2;
    }
  } else if (!date_interval->begin().is_special()) {
    *date_interval = boost::gregorian::date_period(date_interval->begin(), date_interval->begin());
    case_id = 3;
  }

  return case_id;
}
#endif // HOME_TITAN99_CODE_DECADE_SRC_DATE_UTILS_HPP
