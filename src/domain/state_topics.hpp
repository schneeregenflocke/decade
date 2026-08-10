#ifndef STATE_TOPICS_HPP
#define STATE_TOPICS_HPP

#include <QtCore/QObject>
#include <optional>
#include <string>
#include <vector>

#include "calendar_config.hpp"
#include "date_entry.hpp"
#include "date_group.hpp"
#include "font_config.hpp"
#include "page_setup_config.hpp"
#include "scene_snapshot.hpp"
#include "shape_configuration.hpp"
#include "text_edit_view.hpp"
#include "title_config.hpp"

namespace domain {

// The channels a store publishes its state on — the ports in the sense of ports
// and adapters. A store gets its topic injected by reference and publishes on
// it, instead of owning a signal of its own; who owns the topic is the outer
// layer's decision (the EventBus in the application, a local topic in a test).
//
// One class per value, not one template: moc cannot process class templates, so
// a Qt signal has to stand in a concrete QObject. Why that is worth QtCore in
// this layer: AGENTS.md, layer rule 4, and issue #86.
//
// Each carries the value and nothing else, so consumers keep working with
// copyable value objects. `Publish` exists because Qt asks that a signal be
// emitted by the class declaring it; the emit stays here, and the publisher
// calls a plain method.

class DateEntriesTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::vector<DateEntry>& date_entries);

 signals:
  void Published(const std::vector<DateEntry>& date_entries);
};

class DateGroupsTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::vector<DateGroup>& date_groups);

 signals:
  void Published(const std::vector<DateGroup>& date_groups);
};

class PageSetupTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const PageSetupConfig& page_setup);

 signals:
  void Published(const PageSetupConfig& page_setup);
};

class FontConfigTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const FontConfig& font_config);

 signals:
  void Published(const FontConfig& font_config);
};

class TitleConfigTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const TitleConfig& title_config);

 signals:
  void Published(const TitleConfig& title_config);
};

class ShapeConfigSetTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const ShapeConfigSet& shape_config_set);

 signals:
  void Published(const ShapeConfigSet& shape_config_set);
};

class CalendarConfigTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const CalendarConfig& calendar_config);

 signals:
  void Published(const CalendarConfig& calendar_config);
};

class SceneSnapshotTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const SceneNodeSnapshot& scene_snapshot);

 signals:
  void Published(const SceneNodeSnapshot& scene_snapshot);
};

class FilePathTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::string& file_path);

 signals:
  void Published(const std::string& file_path);
};

// The stable path of a scene node; empty means nothing is selected.
class NodePathTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::optional<std::string>& node_path);

 signals:
  void Published(const std::optional<std::string>& node_path);
};

// What is to be seen of a running edit; empty means none.
class TextEditTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(const std::optional<TextEditView>& text_edit);

 signals:
  void Published(const std::optional<TextEditView>& text_edit);
};

// The one topic that carries no state but a bracket around it: true opens a
// burst of changes, false closes it.
class StateBurstTopic : public QObject {
  Q_OBJECT

 public:
  void Publish(bool open);

 signals:
  void Published(bool open);
};

}  // namespace domain

#endif  // STATE_TOPICS_HPP
