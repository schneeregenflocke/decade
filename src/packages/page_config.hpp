#ifndef PAGE_CONFIG_HPP
#define PAGE_CONFIG_HPP

#include <array>
#include <sigslot/signal.hpp>

#include "detail/reentry_guard.hpp"

// Pure domain value. No serialization, no signal -> aggregate, copyable.
struct PageSetupConfig {
  std::array<float, 2> size{};
  std::array<float, 4> margins{};
  int orientation{};
};

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
    const packages::detail::ScopedReentryFlag guard(emitting_);
    page_setup_config = incoming_page_setup_config;
    signal_page_setup_config(page_setup_config);
  }

  void SendPageSetup() { signal_page_setup_config(page_setup_config); }

  [[nodiscard]] const PageSetupConfig& GetPageSetup() const {
    return page_setup_config;
  }

  [[nodiscard]] auto& SignalPageSetupConfig() {
    return signal_page_setup_config;
  }

 private:
  sigslot::signal<const PageSetupConfig&> signal_page_setup_config;
  PageSetupConfig page_setup_config;
  bool emitting_{false};
};
#endif  // PAGE_CONFIG_HPP
