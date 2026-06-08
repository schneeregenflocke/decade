#ifndef CALENDAR_CONFIG_STORE_HPP
#define CALENDAR_CONFIG_STORE_HPP

#include <sigslot/signal.hpp>

#include "calendar_config.hpp"
#include "detail/reentry_guard.hpp"

// Owns a CalendarConfig value plus the change signal and re-entry guard. Has
// identity -> non-copyable. The signal carries the value, so the store needs no
// query delegation: consumers work with the CalendarConfig value directly.
class CalendarConfigStore {
 public:
  CalendarConfigStore() = default;
  ~CalendarConfigStore() = default;
  CalendarConfigStore(const CalendarConfigStore&) = delete;
  CalendarConfigStore(CalendarConfigStore&&) = delete;
  CalendarConfigStore& operator=(const CalendarConfigStore&) = delete;
  CalendarConfigStore& operator=(CalendarConfigStore&&) = delete;

  void ReceiveCalendarConfig(const CalendarConfig& incoming_calendar_config) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    calendar_config_ = incoming_calendar_config;
    signal_calendar_config_(calendar_config_);
  }

  void SendCalendarConfig() { signal_calendar_config_(calendar_config_); }

  [[nodiscard]] const CalendarConfig& GetCalendarConfig() const {
    return calendar_config_;
  }

  sigslot::signal<const CalendarConfig&>& SignalCalendarConfig() {
    return signal_calendar_config_;
  }

 private:
  CalendarConfig calendar_config_;
  sigslot::signal<const CalendarConfig&> signal_calendar_config_;
  bool emitting_{false};
};
#endif  // CALENDAR_CONFIG_STORE_HPP
