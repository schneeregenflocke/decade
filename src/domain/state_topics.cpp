#include "state_topics.hpp"

#include <QtCore/qtmetamacros.h>

#include <optional>
#include <string>
#include <vector>

#include "calendar_config.hpp"
#include "calendar_metrics.hpp"
#include "date_category.hpp"
#include "date_entry.hpp"
#include "font_config.hpp"
#include "page_setup_config.hpp"
#include "scene_snapshot.hpp"
#include "shape_configuration.hpp"
#include "text_edit_view.hpp"
#include "title_config.hpp"

namespace domain {

void DateEntriesTopic::Publish(const std::vector<DateEntry>& date_entries) {
  emit Published(date_entries);
}

void DateCategoriesTopic::Publish(
    const std::vector<DateCategory>& date_categories) {
  emit Published(date_categories);
}

void PageSetupTopic::Publish(const PageSetupConfig& page_setup) {
  emit Published(page_setup);
}

void FontConfigTopic::Publish(const FontConfig& font_config) {
  emit Published(font_config);
}

void TitleConfigTopic::Publish(const TitleConfig& title_config) {
  emit Published(title_config);
}

void ShapeConfigSetTopic::Publish(const ShapeConfigSet& shape_config_set) {
  emit Published(shape_config_set);
}

void CalendarConfigTopic::Publish(const CalendarConfig& calendar_config) {
  emit Published(calendar_config);
}

void CalendarMetricsTopic::Publish(const CalendarMetrics& calendar_metrics) {
  emit Published(calendar_metrics);
}

void SceneSnapshotTopic::Publish(const SceneNodeSnapshot& scene_snapshot) {
  emit Published(scene_snapshot);
}

void FilePathTopic::Publish(const std::string& file_path) {
  emit Published(file_path);
}

void NodePathTopic::Publish(const std::optional<std::string>& node_path) {
  emit Published(node_path);
}

void TextEditTopic::Publish(const std::optional<TextEditView>& text_edit) {
  emit Published(text_edit);
}

void StateBurstTopic::Publish(bool open) { emit Published(open); }

}  // namespace domain
