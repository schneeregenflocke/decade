#include "shape_configuration_store.hpp"

#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "shape_configuration.hpp"
#include "state_topics.hpp"

ShapeConfigurationStore::ShapeConfigurationStore(
    domain::ShapeConfigSetTopic& topic)
    : topic_(topic) {}

void ShapeConfigurationStore::ReceiveShapeConfigSet(
    const ShapeConfigSet& incoming_shape_config_set) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  shape_config_set_ = incoming_shape_config_set;
  topic_.Publish(shape_config_set_);
}

void ShapeConfigurationStore::ReceiveDateGroups(
    const std::vector<DateGroup>& date_groups) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  shape_config_set_.SyncToDateGroups(date_groups.size());
  topic_.Publish(shape_config_set_);
}

void ShapeConfigurationStore::SendShapeConfigSet() {
  topic_.Publish(shape_config_set_);
}

const ShapeConfigSet& ShapeConfigurationStore::Get() const {
  return shape_config_set_;
}
