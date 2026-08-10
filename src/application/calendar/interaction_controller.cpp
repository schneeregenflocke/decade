#include "interaction_controller.hpp"

#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "../../common/debug_log.hpp"
#include "../../domain/state_topic.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"

InteractionController::InteractionController(
    domain::StateTopic<std::optional<PickId>>& hovered_topic,
    domain::StateTopic<std::optional<std::string>>& selected_topic,
    domain::StateTopic<PickId>& edit_requested_topic)
    : hovered_topic_(hovered_topic),
      selected_topic_(selected_topic),
      edit_requested_topic_(edit_requested_topic) {}

void InteractionController::SetPickSource(PickSource pick_source) {
  pick_source_ = std::move(pick_source);
}

void InteractionController::SetPathSource(PathSource path_source) {
  path_source_ = std::move(path_source);
}

void InteractionController::OnPointerMove(glm::vec2 page_point) {
  const std::optional<PickId> hit = Pick(page_point);
  if (hit == hovered_) {
    return;
  }
  hovered_ = hit;

  if (decade_debug::LogEnabled()) {
    if (hovered_.has_value()) {
      std::cout << "hover: " << PickKindName(hovered_->kind) << ' '
                << hovered_->index << '\n';
    } else {
      std::cout << "hover: none\n";
    }
  }

  hovered_topic_(hovered_);
}

void InteractionController::OnPrimaryDown(glm::vec2 page_point) {
  const std::optional<PickId> hit = Pick(page_point);
  if (!hit.has_value() || !path_source_) {
    selected_topic_(std::nullopt);
    return;
  }
  selected_topic_(path_source_(*hit));
}

void InteractionController::OnDoubleClick(glm::vec2 page_point) {
  const std::optional<PickId> hit = Pick(page_point);
  if (!hit.has_value()) {
    return;
  }
  edit_requested_topic_(*hit);
}

std::optional<PickId> InteractionController::Pick(glm::vec2 page_point) const {
  if (!pick_source_) {
    return std::nullopt;
  }
  return pick_source_(page_point);
}
