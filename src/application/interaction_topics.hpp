#ifndef INTERACTION_TOPICS_HPP
#define INTERACTION_TOPICS_HPP

#include <QtCore/QObject>
#include <optional>

#include "../infrastructure/graphics/pick_id.hpp"

namespace application {

// The two channels of pointer interaction. They stand here rather than beside
// the state topics in the domain because they carry a PickId — an
// infrastructure type the domain may not know. Producer and consumers all sit
// in this layer, so nothing is lost by it.

// What the pointer stands over; empty means nothing.
class HoveredTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::optional<PickId>& hovered);

 signals:
  void Published(const std::optional<PickId>& hovered);
};

// A double click on an element: please edit this one.
class EditRequestTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const PickId& picked);

 signals:
  void Published(const PickId& picked);
};

}  // namespace application

#endif  // INTERACTION_TOPICS_HPP
