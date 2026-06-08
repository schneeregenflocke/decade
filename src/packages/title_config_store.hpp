#ifndef TITLE_CONFIG_STORE_HPP
#define TITLE_CONFIG_STORE_HPP

#include <sigslot/signal.hpp>

#include "detail/reentry_guard.hpp"
#include "title_config.hpp"

// Owns a TitleConfig value plus the change signal. Non-copyable. No
// serialization code (handled non-intrusively in the infrastructure layer).
class TitleConfigStore {
 public:
  TitleConfigStore() = default;
  ~TitleConfigStore() = default;
  TitleConfigStore(const TitleConfigStore&) = delete;
  TitleConfigStore& operator=(const TitleConfigStore&) = delete;
  TitleConfigStore(TitleConfigStore&&) = delete;
  TitleConfigStore& operator=(TitleConfigStore&&) = delete;

  void SendTitleConfig() { signal_title_config_(title_config_); }

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    title_config_ = incoming_title_config;
    signal_title_config_(title_config_);
  }

  [[nodiscard]] const TitleConfig& GetTitleConfig() const {
    return title_config_;
  }

  sigslot::signal<const TitleConfig&>& SignalTitleConfig() {
    return signal_title_config_;
  }

 private:
  TitleConfig title_config_;
  sigslot::signal<const TitleConfig&> signal_title_config_;
  bool emitting_{false};
};
#endif  // TITLE_CONFIG_STORE_HPP
