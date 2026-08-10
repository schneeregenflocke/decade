#include "interaction_topics.hpp"

#include <QtCore/qtmetamacros.h>

#include <optional>

#include "../infrastructure/graphics/pick_id.hpp"

namespace application {

void HoveredTopic::Publish(const std::optional<PickId>& hovered) {
  emit Published(hovered);
}

void EditRequestTopic::Publish(const PickId& picked) { emit Published(picked); }

}  // namespace application
