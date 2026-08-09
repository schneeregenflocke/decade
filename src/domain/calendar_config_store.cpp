#include "calendar_config_store.hpp"

#include "calendar_config.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

CalendarConfigStore::CalendarConfigStore(
    domain::StateTopic<CalendarConfig>& topic)
    : topic_(topic) {}

void CalendarConfigStore::ReceiveCalendarConfig(
    const CalendarConfig& incoming_calendar_config) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  calendar_config_ = incoming_calendar_config;
  topic_(calendar_config_);
}

void CalendarConfigStore::SendCalendarConfig() { topic_(calendar_config_); }

const CalendarConfig& CalendarConfigStore::Get() const {
  return calendar_config_;
}
