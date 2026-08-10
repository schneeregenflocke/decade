#ifndef CALENDAR_CONFIG_STORE_HPP
#define CALENDAR_CONFIG_STORE_HPP

#include "calendar_config.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"

// Owns a CalendarConfig value and publishes it on the injected topic. It has
// identity -> not copyable. The topic carries the value, so the store needs no
// query delegation.
class CalendarConfigStore {
 public:
  explicit CalendarConfigStore(domain::CalendarConfigTopic& topic);
  ~CalendarConfigStore() = default;
  CalendarConfigStore(const CalendarConfigStore&) = delete;
  CalendarConfigStore(CalendarConfigStore&&) = delete;
  CalendarConfigStore& operator=(const CalendarConfigStore&) = delete;
  CalendarConfigStore& operator=(CalendarConfigStore&&) = delete;

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config);

  void SendCalendarConfig();

  [[nodiscard]] const CalendarConfig& Get() const;

 private:
  CalendarConfig calendar_config_;
  domain::CalendarConfigTopic& topic_;
  bool emitting_{false};
};
#endif  // CALENDAR_CONFIG_STORE_HPP
