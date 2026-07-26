#ifndef INTERACTION_CONTROLLER_HPP
#define INTERACTION_CONTROLLER_HPP

#include <functional>
#include <glm/vec2.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "../../common/debug_log.hpp"
#include "../../domain/state_topic.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"

// Application: macht aus Zeigereingaben Ereignisse an der Szene. Das Canvas
// liefert Punkte im Seitenraum, der Controller testet sie über eine einsetzbare
// Pick-Quelle (er kennt damit weder CalendarPage noch PhysicsWorld) und meldet:
//
//   * Bewegung -> Hover, nur bei echtem Wechsel;
//   * Klick    -> Auswahl, als Knotenpfad — derselbe Auswahlbegriff, den auch
//                 der Szenenbaum benutzt;
//   * Doppelklick -> Bitte bearbeiten.
//
// Ziehen kommt später an denselben Controller.
class InteractionController {
 public:
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;
  using PathSource = std::function<std::optional<std::string>(PickId)>;

  InteractionController(
      domain::StateTopic<std::optional<PickId>>& hovered_topic,
      domain::StateTopic<std::optional<std::string>>& selected_topic,
      domain::StateTopic<PickId>& edit_requested_topic)
      : hovered_topic_(hovered_topic),
        selected_topic_(selected_topic),
        edit_requested_topic_(edit_requested_topic) {}

  void SetPickSource(PickSource pick_source) {
    pick_source_ = std::move(pick_source);
  }

  // Übersetzt ein getroffenes Element in den Pfad seines Szenenknotens.
  void SetPathSource(PathSource path_source) {
    path_source_ = std::move(path_source);
  }

  void OnPointerMove(glm::vec2 page_point) {
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

  // Ein Klick wählt das getroffene Element; ins Leere hebt die Auswahl auf.
  void OnPrimaryDown(glm::vec2 page_point) {
    const std::optional<PickId> hit = Pick(page_point);
    if (!hit.has_value() || !path_source_) {
      selected_topic_(std::nullopt);
      return;
    }
    selected_topic_(path_source_(*hit));
  }

  void OnDoubleClick(glm::vec2 page_point) {
    const std::optional<PickId> hit = Pick(page_point);
    if (!hit.has_value()) {
      return;
    }
    edit_requested_topic_(*hit);
  }

 private:
  [[nodiscard]] std::optional<PickId> Pick(glm::vec2 page_point) const {
    if (!pick_source_) {
      return std::nullopt;
    }
    return pick_source_(page_point);
  }

  domain::StateTopic<std::optional<PickId>>& hovered_topic_;
  domain::StateTopic<std::optional<std::string>>& selected_topic_;
  domain::StateTopic<PickId>& edit_requested_topic_;
  PickSource pick_source_;
  PathSource path_source_;
  std::optional<PickId> hovered_;
};

#endif  // INTERACTION_CONTROLLER_HPP
