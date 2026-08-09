#ifndef DATE_GROUP_HPP
#define DATE_GROUP_HPP

#include <string>
#include <vector>

// Pure domain value: one date group. No serialization, no signal -> Rule of
// Zero, freely copyable. Persistence is handled non-intrusively in the
// infrastructure layer (see
// infrastructure/persistence/value_serialization.hpp).
class DateGroup {
 public:
  DateGroup() = default;

  explicit DateGroup(std::string name);

  [[nodiscard]] int GetNumber() const;
  void SetNumber(int number);

  [[nodiscard]] const std::string& GetName() const;
  void SetName(std::string name);

 private:
  int number_{0};
  std::string name_{"no name"};
};

// Pure value object: the set of date groups plus the queries that are the
// information expert over that data. No signal, no identity -> Rule of Zero,
// freely copyable. The owning DateGroupStore adds the publish/subscribe and
// re-entry concerns on top.
class DateGroups {
 public:
  // Replace the contents and renumber the groups in order.
  void Assign(const std::vector<DateGroup>& incoming_date_groups);

  [[nodiscard]] const std::vector<DateGroup>& Items() const;

  [[nodiscard]] int GetNumber(const std::string& name) const;

  [[nodiscard]] std::string GetName(int number) const;

  [[nodiscard]] std::vector<std::string> GetDateGroupsNames() const;

  [[nodiscard]] int GetGroupMax() const;

 private:
  void UpdateNumbers();

  std::vector<DateGroup> date_groups_;
};
#endif  // DATE_GROUP_HPP
