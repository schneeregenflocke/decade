#ifndef INTERACTION_CONTROLLER_HPP
#define INTERACTION_CONTROLLER_HPP

#include <functional>
#include <glm/vec2.hpp>
#include <optional>
#include <string>

#include "../../domain/state_topic.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"

// Application: it turns pointer input into events on the scene. The canvas
// delivers points in page space, the controller tests them through an
// injectable pick source (so it knows neither CalendarPage nor PhysicsWorld)
// and reports:
//
//   * movement     -> hover, on a real change alone;
//   * click        -> selection, as a node path — the same notion of selection
//                     the scene tree uses;
//   * double click -> please edit.
//
// Dragging will come to the same controller later.
class InteractionController {
 public:
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;
  using PathSource = std::function<std::optional<std::string>(PickId)>;

  InteractionController(
      domain::StateTopic<std::optional<PickId>>& hovered_topic,
      domain::StateTopic<std::optional<std::string>>& selected_topic,
      domain::StateTopic<PickId>& edit_requested_topic);

  void SetPickSource(PickSource pick_source);

  // Translates a hit element into the path of its scene node.
  void SetPathSource(PathSource path_source);

  void OnPointerMove(glm::vec2 page_point);

  // A click selects the hit element; into the void it cancels the selection.
  void OnPrimaryDown(glm::vec2 page_point);

  void OnDoubleClick(glm::vec2 page_point);

 private:
  [[nodiscard]] std::optional<PickId> Pick(glm::vec2 page_point) const;

  domain::StateTopic<std::optional<PickId>>& hovered_topic_;
  domain::StateTopic<std::optional<std::string>>& selected_topic_;
  domain::StateTopic<PickId>& edit_requested_topic_;
  PickSource pick_source_;
  PathSource path_source_;
  std::optional<PickId> hovered_;
};

#endif  // INTERACTION_CONTROLLER_HPP
