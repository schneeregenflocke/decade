#include "page_setup_store.hpp"

#include "detail/reentry_guard.hpp"
#include "page_setup_config.hpp"
#include "state_topic.hpp"

PageSetupStore::PageSetupStore(domain::StateTopic<PageSetupConfig>& topic)
    : topic_(topic) {}

void PageSetupStore::ReceivePageSetup(
    const PageSetupConfig& incoming_page_setup_config) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  page_setup_config_ = incoming_page_setup_config;
  topic_(page_setup_config_);
}

void PageSetupStore::SendPageSetup() { topic_(page_setup_config_); }

const PageSetupConfig& PageSetupStore::Get() const {
  return page_setup_config_;
}
