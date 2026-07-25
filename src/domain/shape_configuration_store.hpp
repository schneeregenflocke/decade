#ifndef SHAPE_CONFIGURATION_STORE_HPP
#define SHAPE_CONFIGURATION_STORE_HPP

#include <vector>

#include "date_group.hpp"
#include "detail/reentry_guard.hpp"
#include "shape_configuration.hpp"
#include "state_topic.hpp"

// Besitzt einen ShapeConfigSet-Wert und veröffentlicht ihn auf dem
// eingesetzten Topic. Hat Identität -> nicht kopierbar. Das Topic trägt den
// Wert, deshalb braucht der Store keine Query-Delegation.
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

  // Gleicht die gruppenweisen Konfigurationen an die aktuellen Datumsgruppen
  // an (neue aus der Palette ergänzen, verwaiste verwerfen) und veröffentlicht
  // den Satz neu. Diese Palettenlogik ist Domänenwissen und gehört deshalb in
  // den Store, nicht in ein Panel.
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
