#ifndef CALENDAR_CONFIG_STORE_HPP
#define CALENDAR_CONFIG_STORE_HPP

#include <optional>
#include <vector>

#include "calendar_config.hpp"
#include "date_entry.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

// Owns a CalendarConfig value and publishes it on the injected topic. It has
// identity -> not copyable. The topic carries the value, so the store needs no
// query delegation.
//
// The auto span lives here, so every consumer sees the span the page shows:
// while it is on, the store derives the years from the entries it last
// received and overrides whatever span came in.
class CalendarConfigStore {
 public:
  explicit CalendarConfigStore(domain::CalendarConfigTopic& topic);
  ~CalendarConfigStore() = default;
  CalendarConfigStore(const CalendarConfigStore&) = delete;
  CalendarConfigStore(CalendarConfigStore&&) = delete;
  CalendarConfigStore& operator=(const CalendarConfigStore&) = delete;
  CalendarConfigStore& operator=(CalendarConfigStore&&) = delete;

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config);

  void ReceiveDateEntries(const std::vector<DateEntry>& date_entries);

  void SendCalendarConfig();

  [[nodiscard]] const CalendarConfig& Get() const;

 private:
  void ApplyAutoSpan(CalendarConfig& config) const;

  void Adopt(const CalendarConfig& config);

  CalendarConfig calendar_config_;
  std::optional<CalendarSpan::YearSpan> entry_years_;
  domain::CalendarConfigTopic& topic_;
  bool emitting_{false};
};
#endif  // CALENDAR_CONFIG_STORE_HPP
