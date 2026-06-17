#ifndef SHAPE_CONFIGURATION_STORE_HPP
#define SHAPE_CONFIGURATION_STORE_HPP

#include <sigslot/signal.hpp>
#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "shape_configuration.hpp"

// Owns a ShapeConfigSet value plus the change signal and re-entry guard. Has
// identity -> non-copyable. The signal carries the value, so the store needs no
// set-API delegation: consumers work with the ShapeConfigSet value directly.
class ShapeConfigurationStore {
 public:
  ShapeConfigurationStore() = default;
  ~ShapeConfigurationStore() = default;
  ShapeConfigurationStore(const ShapeConfigurationStore&) = delete;
  ShapeConfigurationStore(ShapeConfigurationStore&&) = delete;
  ShapeConfigurationStore& operator=(const ShapeConfigurationStore&) = delete;
  ShapeConfigurationStore& operator=(ShapeConfigurationStore&&) = delete;

  void ReceiveShapeConfigSet(const ShapeConfigSet& incoming_shape_config_set) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    shape_config_set_ = incoming_shape_config_set;
    signal_shape_config_set_(shape_config_set_);
  }

  // Reconciles the dynamic per-group configurations with the current date
  // groups (synthesising defaults for new groups, dropping stale ones) and
  // republishes the set. This palette-synthesis is domain logic, so it lives in
  // the store rather than in a panel.
  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    shape_config_set_.SyncToDateGroups(date_groups.size());
    signal_shape_config_set_(shape_config_set_);
  }

  void SendShapeConfigSet() { signal_shape_config_set_(shape_config_set_); }

  [[nodiscard]] const ShapeConfigSet& GetShapeConfigSet() const {
    return shape_config_set_;
  }

  sigslot::signal<const ShapeConfigSet&>& SignalShapeConfigSet() {
    return signal_shape_config_set_;
  }

 private:
  ShapeConfigSet shape_config_set_;
  sigslot::signal<const ShapeConfigSet&> signal_shape_config_set_;
  bool emitting_{false};
};
#endif  // SHAPE_CONFIGURATION_STORE_HPP
