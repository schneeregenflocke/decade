#ifndef PAGE_SETUP_STORE_HPP
#define PAGE_SETUP_STORE_HPP

#include "detail/reentry_guard.hpp"
#include "page_setup_config.hpp"
#include "state_topic.hpp"

// Owns a PageSetupConfig value and publishes it on the injected topic. Not
// copyable, no serialisation code (that sits non-intrusively in the
// infrastructure).
class PageSetupStore {
 public:
  explicit PageSetupStore(domain::StateTopic<PageSetupConfig>& topic);
  ~PageSetupStore() = default;
  PageSetupStore(const PageSetupStore&) = delete;
  PageSetupStore& operator=(const PageSetupStore&) = delete;
  PageSetupStore(PageSetupStore&&) = delete;
  PageSetupStore& operator=(PageSetupStore&&) = delete;

  void ReceivePageSetup(const PageSetupConfig& incoming_page_setup_config);

  void SendPageSetup();

  [[nodiscard]] const PageSetupConfig& Get() const;

 private:
  PageSetupConfig page_setup_config_;
  domain::StateTopic<PageSetupConfig>& topic_;
  bool emitting_{false};
};
#endif  // PAGE_SETUP_STORE_HPP
