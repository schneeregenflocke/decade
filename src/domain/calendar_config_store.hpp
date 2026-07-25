#ifndef CALENDAR_CONFIG_STORE_HPP
#define CALENDAR_CONFIG_STORE_HPP

#include "calendar_config.hpp"
#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"

// Besitzt einen CalendarConfig-Wert und veröffentlicht ihn auf dem
// eingesetzten Topic. Hat Identität -> nicht kopierbar. Das Topic trägt den
// Wert, deshalb braucht der Store keine Query-Delegation.
class CalendarConfigStore {
 public:
  explicit CalendarConfigStore(domain::StateTopic<CalendarConfig>& topic)
      : topic_(topic) {}
  ~CalendarConfigStore() = default;
  CalendarConfigStore(const CalendarConfigStore&) = delete;
  CalendarConfigStore(CalendarConfigStore&&) = delete;
  CalendarConfigStore& operator=(const CalendarConfigStore&) = delete;
  CalendarConfigStore& operator=(CalendarConfigStore&&) = delete;

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    calendar_config_ = incoming_calendar_config;
    topic_(calendar_config_);
  }

  void SendCalendarConfig() { topic_(calendar_config_); }

  [[nodiscard]] const CalendarConfig& Get() const { return calendar_config_; }

 private:
  CalendarConfig calendar_config_;
  domain::StateTopic<CalendarConfig>& topic_;
  bool emitting_{false};
};
#endif  // CALENDAR_CONFIG_STORE_HPP
