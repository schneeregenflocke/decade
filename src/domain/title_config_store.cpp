#include "title_config_store.hpp"

#include "detail/reentry_guard.hpp"
#include "state_topics.hpp"
#include "title_config.hpp"

TitleConfigStore::TitleConfigStore(domain::TitleConfigTopic& topic)
    : topic_(topic) {}

void TitleConfigStore::ReceiveTitleConfig(
    const TitleConfig& incoming_title_config) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  title_config_ = incoming_title_config;
  topic_.Publish(title_config_);
}

void TitleConfigStore::SendTitleConfig() { topic_.Publish(title_config_); }

const TitleConfig& TitleConfigStore::Get() const { return title_config_; }
