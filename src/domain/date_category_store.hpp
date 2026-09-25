#ifndef DATE_CATEGORY_STORE_HPP
#define DATE_CATEGORY_STORE_HPP

#include <vector>

#include "date_category.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

// Owns a DateCategories value and publishes every change on the injected topic.
// It has identity -> not copyable. It carries no serialisation code.
class DateCategoryStore {
 public:
  explicit DateCategoryStore(domain::DateCategoriesTopic& topic);
  ~DateCategoryStore() = default;
  DateCategoryStore(const DateCategoryStore&) = delete;
  DateCategoryStore& operator=(const DateCategoryStore&) = delete;
  DateCategoryStore(DateCategoryStore&&) = delete;
  DateCategoryStore& operator=(DateCategoryStore&&) = delete;

  void ReceiveDateCategories(
      const std::vector<DateCategory>& incoming_date_categories);

  [[nodiscard]] const DateCategories& Get() const;

  // Call after wiring: it sets the one default category and publishes it, so
  // every consumer holds an initial value.
  void SendDefaultValues();

 private:
  DateCategories date_categories_;
  domain::DateCategoriesTopic& topic_;
  bool emitting_{false};
};
#endif  // DATE_CATEGORY_STORE_HPP
