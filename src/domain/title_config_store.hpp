#ifndef TITLE_CONFIG_STORE_HPP
#define TITLE_CONFIG_STORE_HPP

#include "detail/reentry_guard.hpp"
#include "state_topic.hpp"
#include "title_config.hpp"

// Owns a TitleConfig value and publishes it on the injected topic. Not
// copyable, no serialisation code (that sits non-intrusively in the
// infrastructure).
class TitleConfigStore {
 public:
  explicit TitleConfigStore(domain::StateTopic<TitleConfig>& topic)
      : topic_(topic) {}
  ~TitleConfigStore() = default;
  TitleConfigStore(const TitleConfigStore&) = delete;
  TitleConfigStore& operator=(const TitleConfigStore&) = delete;
  TitleConfigStore(TitleConfigStore&&) = delete;
  TitleConfigStore& operator=(TitleConfigStore&&) = delete;

  void ReceiveTitleConfig(const TitleConfig& incoming_title_config) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    title_config_ = incoming_title_config;
    topic_(title_config_);
  }

  void SendTitleConfig() { topic_(title_config_); }

  [[nodiscard]] const TitleConfig& Get() const { return title_config_; }

 private:
  TitleConfig title_config_;
  domain::StateTopic<TitleConfig>& topic_;
  bool emitting_{false};
};
#endif  // TITLE_CONFIG_STORE_HPP
