#ifndef INTERACTION_CONTROLLER_HPP
#define INTERACTION_CONTROLLER_HPP

#include <functional>
#include <glm/vec2.hpp>
#include <iostream>
#include <optional>
#include <utility>

#include "../../common/debug_log.hpp"
#include "../../domain/state_topic.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"

// Application: macht aus Zeigerbewegung Picking-Ereignisse. Das Canvas liefert
// Punkte im Seitenraum, der Controller testet sie über eine einsetzbare
// Pick-Quelle (er kennt damit weder CalendarPage noch PhysicsWorld) und
// veröffentlicht den Hover nur bei echtem Wechsel. Auswahl und Ziehen kommen
// später an denselben Controller.
class InteractionController {
 public:
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;

  explicit InteractionController(
      domain::StateTopic<std::optional<PickId>>& hovered_topic)
      : hovered_topic_(hovered_topic) {}

  void SetPickSource(PickSource pick_source) {
    pick_source_ = std::move(pick_source);
  }

  void OnPointerMove(glm::vec2 page_point) {
    if (!pick_source_) {
      return;
    }
    const std::optional<PickId> hit = pick_source_(page_point);
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

 private:
  domain::StateTopic<std::optional<PickId>>& hovered_topic_;
  PickSource pick_source_;
  std::optional<PickId> hovered_;
};

#endif  // INTERACTION_CONTROLLER_HPP
