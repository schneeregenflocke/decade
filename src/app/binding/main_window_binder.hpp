#ifndef HOME_TITAN99_CODE_DECADE_SRC_APPLICATION_MAIN_WINDOW_BINDER_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_APPLICATION_MAIN_WINDOW_BINDER_HPP

#include <vector>

#include "../calendar_page.hpp"
#include "../gui/calendar_panel.hpp"
#include "../gui/date_panel.hpp"
#include "../gui/font_panel.hpp"
#include "../gui/groups_panel.hpp"
#include "../gui/opengl_panel.hpp"
#include "../gui/page_panel.hpp"
#include "../gui/shape_panel.hpp"
#include "../gui/title_panel.hpp"
#include "../packages/calendar_config.hpp"
#include "../packages/date_store.hpp"
#include "../packages/group_store.hpp"
#include "../packages/page_config.hpp"
#include "../packages/shape_config.hpp"
#include "../packages/title_config.hpp"
#include "event_bus.hpp"

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
  DateIntervalBundleStore& date_interval_bundle_store;
  TransformDateIntervalBundle& transform_date_interval_bundle;
  PageSetupStore& page_setup_store;
  TitleConfigStore& title_config_store;
  ShapeConfigurationStorage& shape_configuration_storage;
  CalendarConfigStorage& calendar_configuration_storage;

  DateTablePanel& data_table_panel;
  DateGroupsTablePanel& date_groups_table_panel;
  ElementsSetupsPanel& elements_setup_panel;
  PageSetupPanel& page_setup_panel;
  TitleSetupPanel& title_setup_panel;
  CalendarSetupPanel& calendar_setup_panel;
  FontPanel& font_panel;

  CalendarPage& calendar_page;
  GLCanvas& gl_canvas;
};

class MainWindowBinder {
 public:
  static void Bind(EventBus& bus, MainWindowComponents& components) {
    BindDateIntervalBundles(bus, components);
    BindDateGroups(bus, components);
    BindPageSetup(bus, components);
    BindFont(bus, components);
    BindTitleConfig(bus, components);
    BindShapeConfiguration(bus, components);
    BindCalendarConfig(bus, components);
  }

  static void SendInitialValues(MainWindowComponents& components) {
    components.shape_configuration_storage.SendShapeConfigurationStorage();
    components.date_groups_store.SendDefaultValues();
    components.page_setup_panel.SendDefaultValues();
    components.title_setup_panel.SendDefaultValues();
    components.calendar_configuration_storage.SendCalendarConfigStorage();
  }

 private:
  static void BindDateIntervalBundles(EventBus& bus,
                                      MainWindowComponents& components) {
    // Panel -> Store (user edit)
    components.data_table_panel.SignalTableDateIntervalBundles().connect(
        &DateIntervalBundleStore::ReceiveDateIntervalBundles,
        &components.date_interval_bundle_store);

    // Store -> Bus
    components.date_interval_bundle_store.SignalDateIntervalBundles().connect(
        [&bus](const std::vector<DateIntervalBundle>& value) {
          bus.date_interval_bundles(value);
        });

    // Bus -> consumers
    bus.date_interval_bundles.connect(
        &DateTablePanel::ReceiveDateIntervalBundles, &components.data_table_panel);
    bus.date_interval_bundles.connect(
        &TransformDateIntervalBundle::ReceiveDateIntervalBundles,
        &components.transform_date_interval_bundle);

    // Transform adapter -> Bus (transformed topic)
    components.transform_date_interval_bundle.SetTransform(
        {.begin_days = 0, .end_days = 1});
    components.transform_date_interval_bundle
        .SignalTransformDateIntervalBundles()
        .connect([&bus](const std::vector<DateIntervalBundle>& value) {
          bus.transformed_date_interval_bundles(value);
        });

    // Bus (transformed) -> CalendarPage
    bus.transformed_date_interval_bundles.connect(
        &CalendarPage::ReceiveDateIntervalBundles, &components.calendar_page);
  }

  static void BindDateGroups(EventBus& bus, MainWindowComponents& components) {
    components.date_groups_table_panel.SignalTableDateGroups().connect(
        &DateGroupStore::ReceiveDateGroups, &components.date_groups_store);

    components.date_groups_store.SignalDateGroups().connect(
        [&bus](const std::vector<DateGroup>& value) { bus.date_groups(value); });

    bus.date_groups.connect(&DateGroupsTablePanel::ReceiveDateGroups,
                            &components.date_groups_table_panel);
    bus.date_groups.connect(&DateIntervalBundleStore::ReceiveDateGroups,
                            &components.date_interval_bundle_store);
    bus.date_groups.connect(&DateTablePanel::ReceiveDateGroups,
                            &components.data_table_panel);
    bus.date_groups.connect(&ElementsSetupsPanel::ReceiveDateGroups,
                            &components.elements_setup_panel);
    bus.date_groups.connect(&CalendarPage::ReceiveDateGroups,
                            &components.calendar_page);
  }

  static void BindPageSetup(EventBus& bus, MainWindowComponents& components) {
    components.page_setup_panel.signal_page_setup_config.connect(
        &PageSetupStore::ReceivePageSetup, &components.page_setup_store);

    components.page_setup_store.signal_page_setup_config.connect(
        [&bus](const PageSetupConfig& value) { bus.page_setup(value); });

    bus.page_setup.connect(&PageSetupPanel::ReceivePageSetup,
                           &components.page_setup_panel);
    bus.page_setup.connect(&CalendarPage::ReceivePageSetup,
                           &components.calendar_page);
    bus.page_setup.connect(&GLCanvas::ReceivePageSetup, &components.gl_canvas);
  }

  static void BindFont(EventBus& bus, MainWindowComponents& components) {
    components.font_panel.signal_font_filepath.connect(
        [&bus](const std::string& value) { bus.font_filepath(value); });

    bus.font_filepath.connect(&CalendarPage::ReceiveFont,
                              &components.calendar_page);
  }

  static void BindTitleConfig(EventBus& bus,
                              MainWindowComponents& components) {
    components.title_setup_panel.SignalTitleConfig().connect(
        &TitleConfigStore::ReceiveTitleConfig, &components.title_config_store);

    components.title_config_store.SignalTitleConfig().connect(
        [&bus](const TitleConfig& value) { bus.title_config(value); });

    bus.title_config.connect(&TitleSetupPanel::ReceiveTitleConfig,
                             &components.title_setup_panel);
    bus.title_config.connect(&CalendarPage::ReceiveTitleConfig,
                             &components.calendar_page);
  }

  static void BindShapeConfiguration(EventBus& bus,
                                     MainWindowComponents& components) {
    components.elements_setup_panel.SignalShapeConfigurationStorage().connect(
        &ShapeConfigurationStorage::ReceiveShapeConfigurationStorage,
        &components.shape_configuration_storage);

    components.shape_configuration_storage.SignalShapeConfigurationStorage()
        .connect([&bus](const ShapeConfigurationStorage& value) {
          bus.shape_configuration_storage(value);
        });

    bus.shape_configuration_storage.connect(
        &ElementsSetupsPanel::ReceiveShapeConfigurationStorage,
        &components.elements_setup_panel);
    bus.shape_configuration_storage.connect(
        &CalendarPage::ReceiveShapeConfigurationStorage,
        &components.calendar_page);
  }

  static void BindCalendarConfig(EventBus& bus,
                                 MainWindowComponents& components) {
    components.calendar_setup_panel.SignalCalendarConfigStorage().connect(
        &CalendarConfigStorage::ReceiveCalendarConfigStorage,
        &components.calendar_configuration_storage);

    components.calendar_configuration_storage.SignalCalendarConfigStorage()
        .connect([&bus](const CalendarConfigStorage& value) {
          bus.calendar_config_storage(value);
        });

    bus.calendar_config_storage.connect(
        &CalendarSetupPanel::ReceiveCalendarConfigStorage,
        &components.calendar_setup_panel);
    bus.calendar_config_storage.connect(&CalendarPage::ReceiveCalendarConfig,
                                        &components.calendar_page);
  }
};

#endif  // HOME_TITAN99_CODE_DECADE_SRC_APPLICATION_MAIN_WINDOW_BINDER_HPP
