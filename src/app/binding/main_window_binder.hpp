#ifndef MAIN_WINDOW_BINDER_HPP
#define MAIN_WINDOW_BINDER_HPP

#include <glm/vec2.hpp>
#include <optional>
#include <vector>

#include "../../graphics/pick_id.hpp"
#include "../../gui/calendar_panel.hpp"
#include "../../gui/date_panel.hpp"
#include "../../gui/font_panel.hpp"
#include "../../gui/groups_panel.hpp"
#include "../../gui/opengl_panel.hpp"
#include "../../gui/page_panel.hpp"
#include "../../gui/scene_tree_panel.hpp"
#include "../../gui/title_panel.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/calendar_config_store.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_entry_store.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/date_group_store.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/page_setup_store.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/shape_configuration_store.hpp"
#include "../../domain/title_config.hpp"
#include "../../domain/title_config_store.hpp"
#include "../../domain/transform_date_entry.hpp"
#include "calendar_page.hpp"
#include "event_bus.hpp"
#include "interaction_controller.hpp"
#include "scene_snapshot.hpp"

// Centralised wiring between domain stores, presentation panels, the
// rendering adapter, and the GL canvas — all routed through `EventBus`.
//
// Direction conventions:
//   * Panel edits ("upload") are connected directly to the owning store, so a
//     store's `Receive*` is the only place that produces the canonical event.
//   * The store's outgoing signal is forwarded into the bus topic, and every
//     consumer (other stores, panels, the rendering adapter, the GL canvas)
//     subscribes from the bus. This keeps the wiring symmetric and avoids
//     feedback loops, while keeping consumer code unaware of producer identity.
struct MainWindowComponents {
  DateGroupStore& date_groups_store;
  DateEntryStore& date_entry_store;
  TransformDateEntry& transform_date_entry;
  PageSetupStore& page_setup_store;
  TitleConfigStore& title_config_store;
  ShapeConfigurationStore& shape_configuration_store;
  CalendarConfigStore& calendar_configuration_store;

  DateTablePanel& data_table_panel;
  DateGroupsTablePanel& date_groups_table_panel;
  PageSetupPanel& page_setup_panel;
  TitleSetupPanel& title_setup_panel;
  CalendarSetupPanel& calendar_setup_panel;
  FontPanel& font_panel;
  SceneTreePanel& scene_tree_panel;

  CalendarPage& calendar_page;
  GLCanvas& gl_canvas;
  InteractionController& interaction_controller;
};

namespace main_window_binder {

namespace detail {

inline void BindDateEntries(EventBus& bus, MainWindowComponents& components) {
  // Panel -> Store (user edit)
  components.data_table_panel.SignalTableDateEntries().connect(
      &DateEntryStore::ReceiveDateEntries, &components.date_entry_store);

  // Store -> Bus
  components.date_entry_store.SignalDateEntries().connect(
      [&bus](const std::vector<DateEntry>& value) {
        bus.date_entries()(value);
      });

  // Bus -> consumers
  bus.date_entries().connect(&DateTablePanel::ReceiveDateEntries,
                             &components.data_table_panel);
  bus.date_entries().connect(&TransformDateEntry::ReceiveDateEntries,
                             &components.transform_date_entry);

  // Transform adapter -> Bus (transformed topic). No shift: DatePeriod is
  // uniformly half-open [begin, end), so the interval is already exclusive at
  // the end. The previous {end_days = 1} was an inclusive->exclusive fixup left
  // over from the old inclusive-date model; with the half-open model it widened
  // every bar by one day (a 2-day stay rendered as 3 days). The
  // inclusive<->half-open conversion now happens only at the user-facing
  // boundaries (date table, CSV) via PeriodFromInclusiveDates()/Last().
  components.transform_date_entry.SignalTransformDateEntries().connect(
      [&bus](const std::vector<DateEntry>& value) {
        bus.transformed_date_entries()(value);
      });

  // Bus (transformed) -> CalendarPage
  bus.transformed_date_entries().connect(&CalendarPage::ReceiveDateEntries,
                                         &components.calendar_page);
}

inline void BindDateGroups(EventBus& bus, MainWindowComponents& components) {
  components.date_groups_table_panel.SignalTableDateGroups().connect(
      &DateGroupStore::ReceiveDateGroups, &components.date_groups_store);

  components.date_groups_store.SignalDateGroups().connect(
      [&bus](const std::vector<DateGroup>& value) {
        bus.date_groups()(value);
      });

  bus.date_groups().connect(&DateGroupsTablePanel::ReceiveDateGroups,
                            &components.date_groups_table_panel);
  bus.date_groups().connect(&DateEntryStore::ReceiveDateGroups,
                            &components.date_entry_store);
  bus.date_groups().connect(&DateTablePanel::ReceiveDateGroups,
                            &components.data_table_panel);
  // The store synthesises the per-group shape configurations from the palette
  // and republishes the set, so this must run before CalendarPage rebuilds the
  // scene below (which reads the updated configurations off the bus).
  bus.date_groups().connect(&ShapeConfigurationStore::ReceiveDateGroups,
                            &components.shape_configuration_store);
  bus.date_groups().connect(&CalendarPage::ReceiveDateGroups,
                            &components.calendar_page);
}

inline void BindPageSetup(EventBus& bus, MainWindowComponents& components) {
  components.page_setup_panel.SignalPageSetupConfig().connect(
      &PageSetupStore::ReceivePageSetup, &components.page_setup_store);

  components.page_setup_store.SignalPageSetupConfig().connect(
      [&bus](const PageSetupConfig& value) { bus.page_setup()(value); });

  bus.page_setup().connect(&PageSetupPanel::ReceivePageSetup,
                           &components.page_setup_panel);
  bus.page_setup().connect(&CalendarPage::ReceivePageSetup,
                           &components.calendar_page);
  bus.page_setup().connect(&GLCanvas::ReceivePageSetup, &components.gl_canvas);
}

// Font has no domain store — the panel value flows directly to the bus and on
// to the renderer. If font ever needs to be persisted with a project, replace
// this with a Panel -> FontStore -> Bus chain to match the other topics.
inline void BindFont(EventBus& bus, MainWindowComponents& components) {
  components.font_panel.SignalFontFilepath().connect(
      [&bus](const std::string& value) { bus.font_filepath()(value); });

  bus.font_filepath().connect(&CalendarPage::ReceiveFont,
                              &components.calendar_page);
}

inline void BindTitleConfig(EventBus& bus, MainWindowComponents& components) {
  components.title_setup_panel.SignalTitleConfig().connect(
      &TitleConfigStore::ReceiveTitleConfig, &components.title_config_store);

  components.title_config_store.SignalTitleConfig().connect(
      [&bus](const TitleConfig& value) { bus.title_config()(value); });

  bus.title_config().connect(&TitleSetupPanel::ReceiveTitleConfig,
                             &components.title_setup_panel);
  bus.title_config().connect(&CalendarPage::ReceiveTitleConfig,
                             &components.calendar_page);
}

inline void BindShapeConfiguration(EventBus& bus,
                                   MainWindowComponents& components) {
  // The scene tree edits the shape configurations from its detail grid.
  components.scene_tree_panel.SignalShapeConfigSet().connect(
      &ShapeConfigurationStore::ReceiveShapeConfigSet,
      &components.shape_configuration_store);

  components.shape_configuration_store.SignalShapeConfigSet().connect(
      [&bus](const ShapeConfigSet& value) { bus.shape_config_set()(value); });

  bus.shape_config_set().connect(&CalendarPage::ReceiveShapeConfigSet,
                                 &components.calendar_page);
  bus.shape_config_set().connect(&SceneTreePanel::ReceiveShapeConfigSet,
                                 &components.scene_tree_panel);
}

inline void BindCalendarConfig(EventBus& bus,
                               MainWindowComponents& components) {
  components.calendar_setup_panel.SignalCalendarConfig().connect(
      &CalendarConfigStore::ReceiveCalendarConfig,
      &components.calendar_configuration_store);

  components.calendar_configuration_store.SignalCalendarConfig().connect(
      [&bus](const CalendarConfig& value) { bus.calendar_config()(value); });

  bus.calendar_config().connect(&CalendarSetupPanel::ReceiveCalendarConfig,
                                &components.calendar_setup_panel);
  bus.calendar_config().connect(&CalendarPage::ReceiveCalendarConfig,
                                &components.calendar_page);
}

// The rendering adapter is the producer of scene snapshots; the scene-tree
// panel is the only consumer. Routed through the bus like every other topic so
// neither side knows the other.
inline void BindSceneSnapshot(EventBus& bus, MainWindowComponents& components) {
  components.calendar_page.SignalSceneSnapshot().connect(
      [&bus](const SceneNodeSnapshot& value) { bus.scene_snapshot()(value); });

  bus.scene_snapshot().connect(&SceneTreePanel::ReceiveSceneSnapshot,
                               &components.scene_tree_panel);

  // Scene-tree selection -> Bus -> renderer highlight of the node + subtree.
  components.scene_tree_panel.SignalSelectedNode().connect(
      [&bus](const std::optional<std::string>& path) {
        bus.selected_node()(path);
      });
  bus.selected_node().connect(&CalendarPage::ReceiveSelectedNode,
                              &components.calendar_page);
}

// Picking: the canvas reports pointer moves in page space, the controller
// hit-tests them via the rendering adapter, and the resulting hover is
// published on the bus. The visual highlight consumer is wired separately.
inline void BindInteraction(EventBus& bus, MainWindowComponents& components) {
  components.interaction_controller.SetPickSource(
      [page = &components.calendar_page](glm::vec2 point) {
        return page->Pick(point);
      });

  components.gl_canvas.SetPointerMoveCallback(
      [controller = &components.interaction_controller](glm::vec2 point) {
        controller->OnPointerMove(point);
      });

  components.interaction_controller.SignalHovered().connect(
      [&bus](const std::optional<PickId>& hovered) { bus.hovered()(hovered); });

  bus.hovered().connect(&CalendarPage::ReceiveHovered,
                        &components.calendar_page);
}

}  // namespace detail

inline void Bind(EventBus& bus, MainWindowComponents& components) {
  detail::BindDateEntries(bus, components);
  detail::BindDateGroups(bus, components);
  detail::BindPageSetup(bus, components);
  detail::BindFont(bus, components);
  detail::BindTitleConfig(bus, components);
  detail::BindShapeConfiguration(bus, components);
  detail::BindCalendarConfig(bus, components);
  detail::BindSceneSnapshot(bus, components);
  detail::BindInteraction(bus, components);
}

inline void SendInitialValues(MainWindowComponents& components) {
  components.shape_configuration_store.SendShapeConfigSet();
  components.date_groups_store.SendDefaultValues();
  components.page_setup_panel.SendDefaultValues();
  components.title_setup_panel.SendDefaultValues();
  components.calendar_configuration_store.SendCalendarConfig();
}

}  // namespace main_window_binder

#endif  // MAIN_WINDOW_BINDER_HPP
