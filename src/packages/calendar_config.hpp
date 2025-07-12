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

#include "../date_utils.hpp"
#include <sigslot/signal.hpp>

#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

class CalendarSpan {
public:
  CalendarSpan()
      : span(boost::gregorian::date(2000, 1, 1),
             boost::gregorian::date(2010, 1, 1))
  {
    valid_id = CheckAndAdjustDateInterval(&span);
  }

  void SetSpan(const int first_year, const int last_year)
  {
    const boost::gregorian::date min_date =
        boost::gregorian::date(boost::gregorian::min_date_time);
    const boost::gregorian::date max_date =
        boost::gregorian::date(boost::gregorian::max_date_time);

    auto clamped_lower_year =
        std::clamp(first_year, static_cast<int>(min_date.year()),
                   static_cast<int>(max_date.year()));
    auto clamped_upper_year =
        std::clamp(last_year + 1, static_cast<int>(min_date.year()),
                   static_cast<int>(max_date.year()));

    span = boost::gregorian::date_period(
        boost::gregorian::date(clamped_lower_year, 1, 1),
        boost::gregorian::date(clamped_upper_year, 1, 1));

    valid_id = CheckAndAdjustDateInterval(&span);
  }

  bool IsValidSpan() const
  {
    bool is_valid = (CheckDateInterval(span.begin(), span.end()) == 1);
    return is_valid;
  }

  int GetSpanLengthYears() const
  {
    if (IsValidSpan() == false) {
      throw std::runtime_error("Not valid calendar span!");
    }
    return span.end().year() - span.begin().year();
  }

  std::array<int, 2> GetSpanLimitsYears() const
  {
    return std::array<int, 2>{span.begin().year(), span.last().year()};
  }

  std::array<boost::gregorian::date, 2> GetSpanLimitsDate() const
  {
    return std::array<boost::gregorian::date, 2>{span.begin(), span.last()};
  }

  int GetSpanLengthDays() const { return span.length().days(); }

  int GetYear(const int index) const
  {
    int year = span.begin().year() + index;

    if (IsInSpan(year) == false) {
      throw std::logic_error("Year not in span!");
    }

    return year;
  }

  bool IsInSpan(const int year) const
  {
    bool is_in_span = false;

    if (year >= span.begin().year() && year <= span.last().year()) {
      is_in_span = true;
    }
    return is_in_span;
  }

private:
  boost::gregorian::date_period span;
  int valid_id;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(span);
    ar &BOOST_SERIALIZATION_NVP(valid_id);
  }
};

class CalendarConfigStorage : public CalendarSpan {
public:
  CalendarConfigStorage()
      : auto_calendar_span(true),
        spacing_proportions({25, 100, 50, 100, 50, 100, 25})
  {
  }

  void ReceiveCalendarConfigStorage(
      const CalendarConfigStorage &calendar_config_storage)
  {
    *this = calendar_config_storage;
    SendCalendarConfigStorage();
  }

  void SendCalendarConfigStorage() { signal_calendar_config_storage(*this); }

  CalendarConfigStorage &operator=(const CalendarConfigStorage &other)
  {
    auto_calendar_span = other.auto_calendar_span;
    spacing_proportions = other.spacing_proportions;

    CalendarSpan::operator=(other);

    return *this;
  }

  bool auto_calendar_span;
  std::vector<float> spacing_proportions;

  sigslot::signal<const CalendarConfigStorage &> signal_calendar_config_storage;

private:
  friend class boost::serialization::access;
  template <class Archive>
  void save(Archive &ar, const unsigned int version) const
  {
    ar &BOOST_SERIALIZATION_NVP(auto_calendar_span);
    ar &BOOST_SERIALIZATION_NVP(spacing_proportions);
    // boost::serialization::base_object<CalendarSpan>(*this);
    ar &BOOST_SERIALIZATION_BASE_OBJECT_NVP(CalendarSpan);
  }
  template <class Archive> void load(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(auto_calendar_span);
    ar &BOOST_SERIALIZATION_NVP(spacing_proportions);
    // boost::serialization::base_object<CalendarSpan>(*this);
    ar &BOOST_SERIALIZATION_BASE_OBJECT_NVP(CalendarSpan);
    SendCalendarConfigStorage();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};