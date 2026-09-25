#include "date_category_store.hpp"

#include <vector>

#include "date_category.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

DateCategoryStore::DateCategoryStore(domain::DateCategoriesTopic& topic)
    : topic_(topic) {}

void DateCategoryStore::ReceiveDateCategories(
    const std::vector<DateCategory>& incoming_date_categories) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  date_categories_.Assign(incoming_date_categories);
  topic_.Publish(date_categories_.Items());
}

const DateCategories& DateCategoryStore::Get() const {
  return date_categories_;
}

void DateCategoryStore::SendDefaultValues() {
  std::vector<DateCategory> temporary_date_categories;
  temporary_date_categories.emplace_back("Default");
  ReceiveDateCategories(temporary_date_categories);
}
