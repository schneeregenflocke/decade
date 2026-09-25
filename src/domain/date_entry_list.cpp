#include "date_entry_list.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <vector>

#include "date_category.hpp"
#include "date_entry.hpp"
#include "date_period.hpp"

void DateEntryList::Assign(
    const std::vector<DateEntry>& incoming_date_entries) {
  date_entries_.clear();
  date_entries_.reserve(incoming_date_entries.size());

  // Null periods carry no day and are dropped; everything stored is a
  // well-formed half-open interval.
  for (const auto& date_entry : incoming_date_entries) {
    if (!date_entry.GetDateInterval().IsNull()) {
      date_entries_.push_back(date_entry);
    }
  }

  date_entries_.shrink_to_fit();

  Sort();
  ClampCategoriesToKnownRange();
  AssignNumbers();
  AssignInterIntervals();
  AssignCategoryNumbers();
}

void DateEntryList::AssignDateCategories(
    const std::vector<DateCategory>& incoming_date_categories) {
  date_categories_.Assign(incoming_date_categories);
  ClampCategoriesToKnownRange();
}

const std::vector<DateEntry>& DateEntryList::Items() const {
  return date_entries_;
}

bool DateEntryList::IsEmpty() const { return date_entries_.empty(); }

std::size_t DateEntryList::YearSpan() const {
  if (date_entries_.empty()) {
    return 0;
  }
  const int year_span = LastYear() - FirstYear() + 1;
  return static_cast<std::size_t>(year_span);
}

int DateEntryList::FirstYear() const {
  if (date_entries_.empty()) {
    return 0;
  }
  return date_entries_.front().GetDateInterval().Begin().Year();
}

int DateEntryList::LastYear() const {
  if (date_entries_.empty()) {
    return 0;
  }
  // The sort is by Begin(); the latest End() can sit on an earlier-beginning,
  // multi-year entry — so maximise across every entry.
  const auto& latest = std::ranges::max(
      date_entries_, {},
      [](const DateEntry& entry) { return entry.GetDateInterval().Last(); });
  return latest.GetDateInterval().Last().Year();
}

void DateEntryList::Sort() {
  std::ranges::sort(date_entries_, {}, [](const DateEntry& entry) {
    return entry.GetDateInterval().Begin();
  });
}

void DateEntryList::AssignNumbers() {
  int current_number = 0;
  for (auto& date_entry : date_entries_) {
    date_entry.SetNumber(current_number);
    ++current_number;
  }
}

void DateEntryList::AssignInterIntervals() {
  for (auto iterator = date_entries_.begin(); iterator != date_entries_.end();
       ++iterator) {
    const auto next = iterator + 1;
    if (next != date_entries_.end()) {
      iterator->SetDateInterInterval(DatePeriod(
          iterator->GetDateInterval().End(), next->GetDateInterval().Begin()));
    }
  }
}

void DateEntryList::AssignCategoryNumbers() {
  std::map<int, int> categories_counter;

  for (auto& date_entry : date_entries_) {
    const int current_category = date_entry.GetCategory();

    if (categories_counter.contains(current_category)) {
      categories_counter[current_category] += 1;
    } else {
      categories_counter[current_category] = 0;
    }

    date_entry.SetCategoryNumber(categories_counter[current_category]);
  }
}

void DateEntryList::ClampCategoriesToKnownRange() {
  const int category_max = date_categories_.GetCategoryMax();
  for (auto& date_entry : date_entries_) {
    if (date_entry.GetCategory() < 0 ||
        date_entry.GetCategory() > category_max) {
      date_entry.SetCategory(0);
    }
  }
}
