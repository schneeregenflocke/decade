#ifndef INTERACTION_CONTROLLER_HPP
#define INTERACTION_CONTROLLER_HPP

#include <functional>
#include <glm/vec2.hpp>
#include <iostream>
#include <optional>
#include <sigslot/signal.hpp>
#include <utility>

#include "../../infrastructure/graphics/debug_log.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"

// Application: turns pointer movement into picking events. The canvas feeds it
// page-space points; it hit-tests them through a pluggable pick source (so it
// stays unaware of CalendarPage/PhysicsWorld) and emits `hovered` only when the
// hovered element actually changes. Selection and drag will hang off the same
// controller later.
class InteractionController {
 public:
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;

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
        std::cout << "hover: bar " << hovered_->index << '\n';
      } else {
        std::cout << "hover: none\n";
      }
    }

    signal_hovered_(hovered_);
  }

  [[nodiscard]] auto& SignalHovered() { return signal_hovered_; }

 private:
  PickSource pick_source_;
  std::optional<PickId> hovered_;
  sigslot::signal<const std::optional<PickId>&> signal_hovered_;
};

#endif  // INTERACTION_CONTROLLER_HPP
