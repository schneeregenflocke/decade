#ifndef DATE_CATEGORY_HPP
#define DATE_CATEGORY_HPP

#include <string>
#include <vector>

// Pure domain value: one date category. No serialization, no signal -> Rule of
// Zero, freely copyable. Persistence is handled non-intrusively in the
// infrastructure layer (see
// infrastructure/persistence/value_serialization.hpp).
class DateCategory {
 public:
  DateCategory() = default;

  explicit DateCategory(std::string name);

  [[nodiscard]] int GetNumber() const;
  void SetNumber(int number);

  [[nodiscard]] const std::string& GetName() const;
  void SetName(std::string name);

 private:
  int number_{0};
  std::string name_{"no name"};
};

// Pure value object: the set of date categories plus the queries that are the
// information expert over that data. No signal, no identity -> Rule of Zero,
// freely copyable. The owning DateCategoryStore adds the publish/subscribe and
// re-entry concerns on top.
class DateCategories {
 public:
  // Replace the contents and renumber the categories in order.
  void Assign(const std::vector<DateCategory>& incoming_date_categories);

  [[nodiscard]] const std::vector<DateCategory>& Items() const;

  [[nodiscard]] int GetNumber(const std::string& name) const;

  [[nodiscard]] std::string GetName(int number) const;

  [[nodiscard]] std::vector<std::string> GetDateCategoryNames() const;

  [[nodiscard]] int GetCategoryMax() const;

 private:
  void UpdateNumbers();

  std::vector<DateCategory> date_categories_;
};
#endif  // DATE_CATEGORY_HPP
