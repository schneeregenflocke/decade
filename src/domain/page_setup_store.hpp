#ifndef PAGE_SETUP_STORE_HPP
#define PAGE_SETUP_STORE_HPP

#include <sigslot/signal.hpp>

#include "detail/reentry_guard.hpp"
#include "page_setup_config.hpp"

// Owns a PageSetupConfig value plus the change signal. Non-copyable. No
// serialization code (handled non-intrusively in the infrastructure layer).
class PageSetupStore {
 public:
  PageSetupStore() = default;
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
    signal_page_setup_config_(page_setup_config_);
  }

  void SendPageSetup() { signal_page_setup_config_(page_setup_config_); }

  [[nodiscard]] const PageSetupConfig& GetPageSetup() const {
    return page_setup_config_;
  }

  [[nodiscard]] auto& SignalPageSetupConfig() {
    return signal_page_setup_config_;
  }

 private:
  sigslot::signal<const PageSetupConfig&> signal_page_setup_config_;
  PageSetupConfig page_setup_config_;
  bool emitting_{false};
};
#endif  // PAGE_SETUP_STORE_HPP
