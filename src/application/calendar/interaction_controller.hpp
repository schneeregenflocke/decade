#ifndef INTERACTION_CONTROLLER_HPP
#define INTERACTION_CONTROLLER_HPP

#include <cstdint>
#include <functional>
#include <glm/vec2.hpp>
#include <optional>
#include <string>
#include <vector>

#include "../../domain/state_topics.hpp"
#include "../interaction_topics.hpp"
#include "graphics/pick_id.hpp"

// Application: it turns pointer input into events on the scene. The canvas
// delivers points in page space, the controller tests them through an
// injectable pick source (so it knows neither CalendarPage nor PhysicsWorld)
// and reports:
//
//   * movement     -> hover, on a real change alone;
//   * click        -> selection, as node paths — the same notion of selection
//                     the scene tree and the entries table use;
//   * double click -> please edit.
//
// Dragging will come to the same controller later.
class InteractionController {
 public:
  using PickSource = std::function<std::optional<PickId>(glm::vec2)>;
  using PathSource = std::function<std::optional<std::string>(PickId)>;

  // A plain click replaces the selection; a click with Shift toggles the hit
  // element in it and leaves the rest standing.
  enum class Gesture : std::uint8_t { kReplace, kToggle };

  InteractionController(application::HoveredTopic& hovered_topic,
                        domain::NodePathsTopic& selected_topic,
                        application::EditRequestTopic& edit_requested_topic);

  void SetPickSource(PickSource pick_source);

  // Translates a hit element into the path of its scene node.
  void SetPathSource(PathSource path_source);

  // The selection as it stands, whoever changed it last; a toggle starts from
  // it.
  void ReceiveSelectedNodes(const std::vector<std::string>& paths);

  void OnPointerMove(glm::vec2 page_point);

  // A click on nothing cancels the selection, unless it toggles.
  void OnPrimaryDown(glm::vec2 page_point, Gesture gesture);

  void OnDoubleClick(glm::vec2 page_point);

 private:
  [[nodiscard]] std::optional<PickId> Pick(glm::vec2 page_point) const;

  [[nodiscard]] std::optional<std::string> PathAt(glm::vec2 page_point) const;

  application::HoveredTopic& hovered_topic_;
  domain::NodePathsTopic& selected_topic_;
  application::EditRequestTopic& edit_requested_topic_;
  PickSource pick_source_;
  PathSource path_source_;
  std::optional<PickId> hovered_;
  std::vector<std::string> selected_paths_;
};

#endif  // INTERACTION_CONTROLLER_HPP
