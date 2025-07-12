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

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>


#include <algorithm>
#include <string>
#include <vector>

class DateGroup {
public:
  DateGroup() : name("no name"), number(0), exclude(false) {}

  DateGroup(std::string name) : name(name), number(0), exclude(false) {}

  int number;
  std::string name;
  bool exclude;

private:
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(number);
    ar &BOOST_SERIALIZATION_NVP(name);
    ar &BOOST_SERIALIZATION_NVP(exclude);
  }
};

class DateGroupStore {
public:
  void ReceiveDateGroups(const std::vector<DateGroup> &date_groups)
  {
    this->date_groups = date_groups;
    UpdateNumbers();
    signal_date_groups(date_groups);
  }

  std::vector<DateGroup> GetDateGroups() const { return date_groups; }

  // call after connecting
  void SendDefaultValues()
  {
    std::vector<DateGroup> temporary_date_groups;
    temporary_date_groups.push_back(DateGroup("Default"));
    ReceiveDateGroups(temporary_date_groups);
  }

  int GetNumber(const std::string &name) const
  {
    auto find_lambda = [&](const DateGroup &compare) { return compare.name == name; };
    auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->number;
    } else {
      throw std::runtime_error("number not found");
    }
  }

  std::string GetName(int number) const
  {
    auto find_lambda = [&](const DateGroup &compare) { return compare.number == number; };
    auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->name;
    } else {
      throw std::runtime_error("string not found");
    }
  }

  std::vector<std::string> GetDateGroupsNames() const
  {
    std::vector<std::string> name_strings(date_groups.size());
    auto iterator = name_strings.begin();
    for (const auto &date_group : date_groups) {
      *iterator = date_group.name;
      ++iterator;
    }
    return name_strings;
  }

  int GetGroupMax() const { return date_groups.size() - 1; }

  bool GetExclude(int number) const { return date_groups[number].exclude; }

  sigslot::signal<const std::vector<DateGroup> &> signal_date_groups;

private:
  friend class boost::serialization::access;
  template <class Archive> void save(Archive &ar, const unsigned int version) const
  {
    ar &BOOST_SERIALIZATION_NVP(date_groups);
  }
  template <class Archive> void load(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(date_groups);
    signal_date_groups(date_groups);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  void UpdateNumbers()
  {
    int number = 0;
    for (auto &date_group : date_groups) {
      date_group.number = number;
      ++number;
    }
  }
  std::vector<DateGroup> date_groups;
};
