#ifndef HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_GROUP_STORE_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_GROUP_STORE_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <sigslot/signal.hpp>


class DateGroup {
public:
  DateGroup() = default;

  explicit DateGroup(std::string name) : name_(std::move(name)) {}

  [[nodiscard]] int GetNumber() const { return number_; }
  void SetNumber(int number) { number_ = number; }

  [[nodiscard]] const std::string &GetName() const { return name_; }
  void SetName(std::string name) { name_ = std::move(name); }

  [[nodiscard]] bool IsExcluded() const { return exclude_; }
  void SetExcluded(bool exclude) { exclude_ = exclude; }

private:
  int number_{0};
  std::string name_{"no name"};
  bool exclude_{false};

  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &archive, const unsigned int version)
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(number_);
    archive &BOOST_SERIALIZATION_NVP(name_);
    archive &BOOST_SERIALIZATION_NVP(exclude_);
  }
};

class DateGroupStore {
public:
  void ReceiveDateGroups(const std::vector<DateGroup> &incoming_date_groups)
  {
    date_groups = incoming_date_groups;
    UpdateNumbers();
    signal_date_groups(date_groups);
  }

  [[nodiscard]] const std::vector<DateGroup> &GetDateGroups() const { return date_groups; }

  // call after connecting
  void SendDefaultValues()
  {
    std::vector<DateGroup> temporary_date_groups;
    temporary_date_groups.emplace_back("Default");
    ReceiveDateGroups(temporary_date_groups);
  }

  [[nodiscard]] int GetNumber(const std::string &name) const
  {
    auto find_lambda = [&](const DateGroup &compare) { return compare.GetName() == name; };
    auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->GetNumber();
    }
    throw std::runtime_error("number not found");
  }

  [[nodiscard]] std::string GetName(int number) const
  {
    auto find_lambda = [&](const DateGroup &compare) { return compare.GetNumber() == number; };
    auto found = std::find_if(date_groups.cbegin(), date_groups.cend(), find_lambda);
    if (found != date_groups.end()) {
      return found->GetName();
    }
    throw std::runtime_error("string not found");
  }

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const
  {
    std::vector<std::string> name_strings(date_groups.size());
    auto iterator = name_strings.begin();
    for (const auto &date_group : date_groups) {
      *iterator = date_group.GetName();
      ++iterator;
    }
    return name_strings;
  }

  [[nodiscard]] int GetGroupMax() const
  {
    if (date_groups.empty()) {
      return -1;
    }
    return static_cast<int>(date_groups.size()) - 1;
  }

  [[nodiscard]] bool GetExclude(int number) const
  {
    return date_groups.at(static_cast<size_t>(number)).IsExcluded();
  }

  sigslot::signal<const std::vector<DateGroup> &> &SignalDateGroups()
  {
    return signal_date_groups;
  }

private:
  friend class boost::serialization::access;
  template <class Archive> void save(Archive &archive, const unsigned int version) const
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(date_groups);
  }
  template <class Archive> void load(Archive &archive, const unsigned int version)
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(date_groups);
    signal_date_groups(date_groups);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  void UpdateNumbers()
  {
    int number = 0;
    for (auto &date_group : date_groups) {
      date_group.SetNumber(number);
      ++number;
    }
  }
  std::vector<DateGroup> date_groups;
  sigslot::signal<const std::vector<DateGroup> &> signal_date_groups;
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_GROUP_STORE_HPP
