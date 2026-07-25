#ifndef PAGE_SETUP_STORE_HPP
#define PAGE_SETUP_STORE_HPP

#include "detail/reentry_guard.hpp"
#include "page_setup_config.hpp"
#include "state_topic.hpp"

// Besitzt einen PageSetupConfig-Wert und veröffentlicht ihn auf dem
// eingesetzten Topic. Nicht kopierbar, kein Serialisierungscode (der liegt
// nicht-intrusiv in der Infrastructure).
class PageSetupStore {
 public:
  explicit PageSetupStore(domain::StateTopic<PageSetupConfig>& topic)
      : topic_(topic) {}
  ~PageSetupStore() = default;
  PageSetupStore(const PageSetupStore&) = delete;
  PageSetupStore& operator=(const PageSetupStore&) = delete;
  PageSetupStore(PageSetupStore&&) = delete;
  PageSetupStore& operator=(PageSetupStore&&) = delete;

  void ReceivePageSetup(const PageSetupConfig& incoming_page_setup_config) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    page_setup_config_ = incoming_page_setup_config;
    topic_(page_setup_config_);
  }

  void SendPageSetup() { topic_(page_setup_config_); }

  [[nodiscard]] const PageSetupConfig& Get() const {
    return page_setup_config_;
  }

 private:
  PageSetupConfig page_setup_config_;
  domain::StateTopic<PageSetupConfig>& topic_;
  bool emitting_{false};
};
#endif  // PAGE_SETUP_STORE_HPP
