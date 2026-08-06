#ifndef SHAPE_CONFIGURATION_STORE_HPP
#define SHAPE_CONFIGURATION_STORE_HPP

#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "shape_configuration.hpp"
#include "state_topic.hpp"

// Owns a ShapeConfigSet value and publishes it on the injected topic. It has
// identity -> not copyable. The topic carries the value, so the store needs no
// query delegation.
class ShapeConfigurationStore {
 public:
  explicit ShapeConfigurationStore(domain::StateTopic<ShapeConfigSet>& topic)
      : topic_(topic) {}
  ~ShapeConfigurationStore() = default;
  ShapeConfigurationStore(const ShapeConfigurationStore&) = delete;
  ShapeConfigurationStore(ShapeConfigurationStore&&) = delete;
  ShapeConfigurationStore& operator=(const ShapeConfigurationStore&) = delete;
  ShapeConfigurationStore& operator=(ShapeConfigurationStore&&) = delete;

  void ReceiveShapeConfigSet(const ShapeConfigSet& incoming_shape_config_set) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    shape_config_set_ = incoming_shape_config_set;
    topic_(shape_config_set_);
  }

  // Aligns the per-group configurations to the current date groups (add new
  // ones from the palette, discard orphans) and publishes the set anew. This
  // palette logic is domain knowledge and therefore belongs in the store, not
  // in a panel.
  void ReceiveDateGroups(const std::vector<DateGroup>& date_groups) {
    if (emitting_) {
      return;
    }
    const domain::detail::ScopedReentryFlag guard(emitting_);
    shape_config_set_.SyncToDateGroups(date_groups.size());
    topic_(shape_config_set_);
  }

  void SendShapeConfigSet() { topic_(shape_config_set_); }

  [[nodiscard]] const ShapeConfigSet& Get() const { return shape_config_set_; }

 private:
  ShapeConfigSet shape_config_set_;
  domain::StateTopic<ShapeConfigSet>& topic_;
  bool emitting_{false};
};
#endif  // SHAPE_CONFIGURATION_STORE_HPP
