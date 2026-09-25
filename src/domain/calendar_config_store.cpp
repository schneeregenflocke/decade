#include "calendar_config_store.hpp"

#include <optional>
#include <vector>

#include "calendar_config.hpp"
#include "date_entry.hpp"
#include "date_entry_list.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

CalendarConfigStore::CalendarConfigStore(domain::CalendarConfigTopic& topic)
    : topic_(topic) {}

void CalendarConfigStore::ReceiveCalendarConfig(
    const CalendarConfig& incoming_calendar_config) {
  CalendarConfig config = incoming_calendar_config;
  FitYearsToEntries(config);
  Adopt(config);
}

void CalendarConfigStore::ReceiveDateEntries(
    const std::vector<DateEntry>& date_entries) {
  DateEntryList entries;
  entries.Assign(date_entries);
  entry_years_ = std::nullopt;
  if (!entries.IsEmpty()) {
    entry_years_ = CalendarSpan::YearSpan{.first_year = entries.FirstYear(),
                                          .last_year = entries.LastYear()};
  }

  CalendarConfig config = calendar_config_;
  FitYearsToEntries(config);
  // Entries change far more often than their year range; an unchanged span
  // would only cost every consumer a rebuild.
  if (config.FirstYear() != calendar_config_.FirstYear() ||
      config.LastYear() != calendar_config_.LastYear()) {
    Adopt(config);
  }
}

void CalendarConfigStore::SendCalendarConfig() {
  topic_.Publish(calendar_config_);
}

const CalendarConfig& CalendarConfigStore::Get() const {
  return calendar_config_;
}

void CalendarConfigStore::FitYearsToEntries(CalendarConfig& config) const {
  if (config.IsFitYearsToEntries() && entry_years_.has_value()) {
    config.SetYears(*entry_years_);
  }
}

void CalendarConfigStore::Adopt(const CalendarConfig& config) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  calendar_config_ = config;
  topic_.Publish(calendar_config_);
}
