/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <array>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <cctype>
#include <string>

struct date_format_descriptor {
  std::array<size_t, 3> date_order;
  std::array<size_t, 3> delimiters;
  bool shortyear;
};

inline date_format_descriptor InitDateFormat()
{
  const int testyear = 1999;
  const int testyearshort = 99;
  const int testmonth = 4;
  const int testday = 23;

  std::tm test_tm = boost::gregorian::to_tm(boost::gregorian::date(testyear, testmonth, testday));
  std::ostringstream ostrm;
  ostrm << std::put_time(&test_tm, "%x");
  std::string date_string = ostrm.str();

  date_format_descriptor date_format;

  size_t delimPos = 0;
  for (size_t index = 0; index < 3; ++index) {
    date_string = date_string.substr(delimPos, std::string::npos);
    int number = std::stoi(date_string, &delimPos);
    date_format.delimiters[index] = date_string[delimPos];
    ++delimPos;

    if (number == testyear) {
      date_format.date_order[index] = 0;
      date_format.shortyear = false;
    }

    if (number == testyearshort) {
      date_format.date_order[index] = 0;
      date_format.shortyear = true;
    }

    if (number == testmonth) {
      date_format.date_order[index] = 1;
    }

    if (number == testday) {
      date_format.date_order[index] = 2;
    }
  }

  return date_format;
}

inline std::string boost_date_to_string(const boost::gregorian::date &date_variable)
{
  std::ostringstream ostrm;

  if (!date_variable.is_special()) {
    std::tm datetm = boost::gregorian::to_tm(date_variable);
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
  std::array<int, 3> date_parts;
  size_t delimPos = 0;
  boost::gregorian::date date_variable;

  for (size_t index = 0; index < 3; ++index) {
    if (delimPos > date_string.length() || date_string.length() == 0) {
      failed = true;
      break;
    }

    non_const_date_string = non_const_date_string.substr(delimPos, std::string::npos);

    if (!std::isdigit(date_string[0])) {
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

    if (format.date_order[index] == 0 && format.shortyear) {
      if (number >= 0 && number < 50) {
        number += 2000;
      }

      if (number >= 50 && number < 100) {
        number += 1900;
      }
    }

    date_parts[format.date_order[index]] = number;
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

  if (begin_date.is_special() == false && end_date.is_special() == false) {
    boost::gregorian::date_period period = boost::gregorian::date_period(begin_date, end_date);

    if (period.is_null() == false) {
      case_id = 1;
    } else if (begin_date == end_date) {
      case_id = 2;
    }
  } else if (begin_date.is_special() == false) {
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

  if (date_interval->begin().is_special() == false && date_interval->end().is_special() == false) {
    if (date_interval->is_null() == false) {
      case_id = 1;
    } else if (date_interval->begin() == date_interval->end()) {
      case_id = 2;
    }
  } else if (date_interval->begin().is_special() == false) {
    *date_interval = boost::gregorian::date_period(date_interval->begin(), date_interval->begin());
    case_id = 3;
  }

  return case_id;
}
