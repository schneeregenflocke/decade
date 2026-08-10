#include "calendar_page.hpp"

#include <chrono>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <ratio>
#include <string>
#include <vector>

#include "../../common/debug_log.hpp"
#include "../../domain/calendar_config.hpp"
#include "../../domain/date_entry.hpp"
#include "../../domain/date_group.hpp"
#include "../../domain/font_config.hpp"
#include "../../domain/page_setup_config.hpp"
#include "../../domain/shape_configuration.hpp"
#include "../../domain/state_topics.hpp"
#include "../../domain/text_edit_view.hpp"
#include "../../domain/title_config.hpp"
#include "../../infrastructure/graphics/font.hpp"
#include "../../infrastructure/graphics/graphics_engine.hpp"
#include "../../infrastructure/graphics/page_geometry.hpp"
#include "../../infrastructure/graphics/pick_id.hpp"
#include "../render_surface.hpp"

CalendarPage::CalendarPage(GraphicsEngine& graphics_engine,
                           application::RenderSurface& render_surface,
                           const FontConfig& font_config,
                           domain::SceneSnapshotTopic& snapshot_topic)
    : render_surface_(render_surface),
      snapshot_topic_(snapshot_topic),
      font_config_(font_config),
      font_(std::make_shared<Font>(font_config.FilePath())),
      scene_composer_(graphics_engine, scene_, font_, font_config_, page_size_,
                      page_margin_, title_config_, calendar_config_,
                      shape_config_, date_groups_, date_entry_bars_) {}

void CalendarPage::ReceiveDateGroups(
    const std::vector<DateGroup>& date_groups_in) {
  date_groups_.Assign(date_groups_in);
  date_entry_bars_.ReceiveDateGroups(date_groups_in);
  Update();
}

void CalendarPage::ReceiveDateEntries(
    const std::vector<DateEntry>& date_entries) {
  date_entry_bars_.ReceiveDateEntries(date_entries);
  Update();
}

void CalendarPage::ReceivePageSetup(const PageSetupConfig& page_setup_config) {
  page_size_ = PageRect(page_setup_config);
  page_margin_ = PageMarginRect(page_setup_config);
  Update();
}

void CalendarPage::ReceiveFont(const FontConfig& font_config) {
  if (font_config.FilePath() != font_config_.FilePath()) {
    // A font is GL: it rasters its glyphs into a texture.
    render_surface_.MakeGraphicsCurrent();
    font_ = std::make_shared<Font>(font_config.FilePath());
  }
  font_config_ = font_config;
  Update();
}

void CalendarPage::ReceiveTitleConfig(
    const TitleConfig& incoming_title_config) {
  title_config_ = incoming_title_config;
  Update();
}

void CalendarPage::ReceiveCalendarConfig(
    const CalendarConfig& incoming_calendar_config) {
  calendar_config_ = incoming_calendar_config;
  Update();
}

void CalendarPage::ReceiveShapeConfigSet(
    const ShapeConfigSet& incoming_shape_config_set) {
  shape_config_ = incoming_shape_config_set;
  Update();
}

void CalendarPage::ReceiveStateBurst(bool open) {
  if (open) {
    ++open_bursts_;
    return;
  }
  if (open_bursts_ > 0) {
    --open_bursts_;
  }
  if (open_bursts_ == 0 && pending_update_) {
    pending_update_ = false;
    Rebuild();
  }
}

void CalendarPage::Update() {
  if (open_bursts_ > 0) {
    pending_update_ = true;
    return;
  }
  Rebuild();
}

std::optional<PickId> CalendarPage::Pick(glm::vec2 page_point) const {
  return physics_world_.Raycast(page_point);
}

void CalendarPage::ReceiveHovered(const std::optional<PickId>& hovered) {
  scene_composer_.SetHovered(hovered);
  render_surface_.Repaint();
}

void CalendarPage::ReceiveSelectedNode(const std::optional<std::string>& path) {
  scene_composer_.SetSelectedNode(path);
  render_surface_.Repaint();
}

void CalendarPage::ReceiveTextEdit(
    const std::optional<TextEditView>& text_edit) {
  scene_composer_.SetTextEdit(text_edit);
  BuildScene("text edit");
  render_surface_.Repaint();
}

std::optional<std::string> CalendarPage::NodePathFor(
    const PickId& picked) const {
  return scene_composer_.NodePathFor(picked);
}

std::size_t CalendarPage::TitleCaretIndexAt(glm::vec2 page_point) const {
  return scene_composer_.TitleCaretIndexAt(page_point);
}

void CalendarPage::Rebuild() {
  BuildScene("state change");
  physics_world_.Rebuild(scene_composer_.PickBoxes());
  render_surface_.RefreshView();
  snapshot_topic_.Publish(scene_composer_.SceneSnapshot());
}

void CalendarPage::BuildScene(const char* reason) {
  render_surface_.MakeGraphicsCurrent();
  const auto started = std::chrono::steady_clock::now();
  scene_composer_.Build();
  if (decade_debug::LogEnabled()) {
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started);
    std::cout << "scene build #" << ++build_count_ << " (" << reason << ") "
              << elapsed.count() << " ms\n";
  }
}
