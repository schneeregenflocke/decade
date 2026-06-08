#ifndef CALENDAR_SCENE_BUILDER_HPP
#define CALENDAR_SCENE_BUILDER_HPP

#include <array>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/frame_layout.hpp"
#include "../../graphics/graphics_engine.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/shapes.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/date_interval_bundle_bar_store.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/title_config.hpp"

// Builds and fills the calendar scene graph from domain state. This is the
// rendering/layout half of the former CalendarPage: it owns the scene graph and
// translates the (referenced) domain state into shapes. It is GL-canvas free —
// it only knows about the GraphicsEngine and the scene graph. CalendarPage owns
// the state and drives Build() in reaction to store updates.
class CalendarSceneBuilder {
 public:
  CalendarSceneBuilder(GraphicsEngine* graphics_engine_in,
                       const std::shared_ptr<Font>& font_in,
                       const rectf& page_size_in, const rectf& page_margin_in,
                       const TitleConfig& title_config_in,
                       CalendarConfig& calendar_config_in,
                       const ShapeConfigSet& shape_config_in,
                       const DateGroups& date_groups_in,
                       const DateIntervalBundleBarStore& data_store_in)
      : scene_graph_(std::make_shared<SceneNode>("root")),
        graphics_engine_(graphics_engine_in),
        font_(font_in),
        page_size_(page_size_in),
        page_margin_(page_margin_in),
        title_config_(title_config_in),
        calendar_config_(calendar_config_in),
        shape_config_(shape_config_in),
        date_groups_(date_groups_in),
        data_store_(data_store_in) {
    graphics_engine_->set_scene_graph(scene_graph_);
    auto* simple_shader =
        graphics_engine_->search_shader("Simple Shader").value_or(nullptr);
    auto* rectangles_shader =
        graphics_engine_->search_shader("Rectangles Shader").value_or(nullptr);
    auto* font_shader =
        graphics_engine_->search_shader("Font Shader").value_or(nullptr);

    auto page_shape = std::make_shared<QuadrilateralShape>(simple_shader);
    auto page_node = std::make_shared<SceneNode>("page", page_shape);
    scene_graph_->add_child(page_node);

    auto print_area_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto print_area_node =
        std::make_shared<SceneNode>("print area", print_area_shape);
    page_node->add_child(print_area_node);

    auto title_area_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto title_area_node =
        std::make_shared<SceneNode>("title area", title_area_shape);
    print_area_node->add_child(title_area_node);

    auto row_labels_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto row_labels_node =
        std::make_shared<SceneNode>("row label area", row_labels_shape);
    print_area_node->add_child(row_labels_node);

    auto column_labels_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto column_labels_node =
        std::make_shared<SceneNode>("column label area", column_labels_shape);
    print_area_node->add_child(column_labels_node);

    auto years_cells_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto years_cells_node =
        std::make_shared<SceneNode>("year cells", years_cells_shape);
    print_area_node->add_child(years_cells_node);

    auto months_cells_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto months_cells_node =
        std::make_shared<SceneNode>("month cells", months_cells_shape);
    print_area_node->add_child(months_cells_node);

    auto days_cells0_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto days_cells0_node =
        std::make_shared<SceneNode>("day cells 0", days_cells0_shape);
    print_area_node->add_child(days_cells0_node);

    auto days_cells1_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto days_cells1_node =
        std::make_shared<SceneNode>("day cells 1", days_cells1_shape);
    print_area_node->add_child(days_cells1_node);

    auto bars_cells_node = std::make_shared<SceneNode>("bar cells");
    print_area_node->add_child(bars_cells_node);

    auto years_totals_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    auto years_totals_node =
        std::make_shared<SceneNode>("year total cells", years_totals_shape);
    print_area_node->add_child(years_totals_node);

    auto years_totals_text_node =
        std::make_shared<SceneNode>("year total text");
    print_area_node->add_child(years_totals_text_node);

    auto legend_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto legend_shape_node =
        std::make_shared<SceneNode>("legend area", legend_shape);
    print_area_node->add_child(legend_shape_node);

    auto legend_entries_node = std::make_shared<SceneNode>("legend entries");
    print_area_node->add_child(legend_entries_node);

    auto legend_text_node = std::make_shared<SceneNode>("legend text");
    print_area_node->add_child(legend_text_node);

    auto title_font_shape = std::make_shared<FontShape>(font_shader);
    title_font_shape->set_font(font_);
    auto title_font_node =
        std::make_shared<SceneNode>("title text", title_font_shape);
    print_area_node->add_child(title_font_node);

    auto month_text_node = std::make_shared<SceneNode>("month text");
    print_area_node->add_child(month_text_node);

    auto year_text_node = std::make_shared<SceneNode>("year text");
    print_area_node->add_child(year_text_node);

    auto bar_labels_node = std::make_shared<SceneNode>("bar labels");
    print_area_node->add_child(bar_labels_node);
  }

  void Build() {
    auto node = scene_graph_->search_node("page").value_or(nullptr);
    if (!node) {
      return;
    }
    auto shape =
        std::dynamic_pointer_cast<QuadrilateralShape>(node->get_shape());
    if (!shape) {
      return;
    }
    shape->set_shape(page_size_);
    shape->set_color(glm::vec4(kOne, kOne, kOne, kOne));

    print_area_ = page_size_.reduce(page_margin_);

    title_frame_ = print_area_;
    title_frame_.setB(title_frame_.t() - title_config_.FrameHeight());

    page_margin_frame_ = print_area_;
    page_margin_frame_.setT(title_frame_.b());
    // const rectf page_margin_frame_margin(0.0f, 0.0f, 0.0f, 0.f);
    // page_margin_frame = page_margin_frame.reduce(page_margin_frame_margin);

    calendar_frame_ = page_margin_frame_;
    const rectf calendar_frame_margin =
        rectf(kZero, kDefaultMargin, kZero, kZero);
    calendar_frame_ = calendar_frame_.reduce(calendar_frame_margin);

    if (calendar_config_.IsAutoCalendarSpan() && !data_store_.is_empty()) {
      calendar_config_.SetSpan(
          CalendarSpan::YearSpan{.first_year = data_store_.GetFirstYear(),
                                 .last_year = data_store_.GetLastYear()});
    }

    const size_t additional_rows = 2;
    const size_t number_rows =
        additional_rows + calendar_config_.GetSpanLengthYears();

    cell_width_ = calendar_frame_.width() / kCalendarColumns;
    row_height_ = calendar_frame_.height() / static_cast<float>(number_rows);

    const rectf cells_frame_margin(cell_width_, kZero,
                                   row_height_ * kRowHeaderScale, kZero);
    cells_frame_ = calendar_frame_.reduce(cells_frame_margin);

    proportion_frame_layout_.SetupRowFrames(
        cells_frame_, calendar_config_.GetSpanLengthYears());
    proportion_frame_layout_.SetupSubFrames(
        calendar_config_.GetSpacingProportions());

    day_width_ = cells_frame_.width() / kDaysPerYear;

    x_labels_frame_ = calendar_frame_.reduce(
        rectf(cell_width_, kZero, row_height_, cells_frame_.height()));
    y_labels_frame_ = calendar_frame_.reduce(rectf(
        kZero, cells_frame_.width(), row_height_ * kRowHeaderScale, kZero));
    legend_frame_ = calendar_frame_.reduce(
        rectf(cell_width_, kZero, kZero, cells_frame_.height() + row_height_));

    SetupPrintAreaShape();
    SetupTitleShape();
    SetupCalendarLabelsShape();
    SetupDaysShapes();
    SetupMonthsShapes();
    SetupYearsShapes();
    SetupBarsShape();
    SetupYearsTotals();
    SetupLegend();
  }

 private:
  void SetupPrintAreaShape() {
    auto config = shape_config_.GetShapeConfiguration("Page Margin");

    auto node = scene_graph_->search_node("print area").value_or(nullptr);
    if (!node) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    if (!shape) {
      return;
    }

    shape->set_shape(print_area_, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupTitleShape() {
    auto config = shape_config_.GetShapeConfiguration("Title Frame");

    auto node = scene_graph_->search_node("title area").value_or(nullptr);
    if (!node) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    if (!shape) {
      return;
    }

    shape->set_shape(title_frame_, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});

    auto title_node = scene_graph_->search_node("title text").value_or(nullptr);
    if (!title_node) {
      return;
    }
    auto title_shape =
        std::dynamic_pointer_cast<FontShape>(title_node->get_shape());
    if (!title_shape) {
      return;
    }
    title_shape->set_font(font_);
    const std::array<float, 4>& text_color = title_config_.TextColor();
    title_shape->set_color(
        glm::vec4(text_color[0], text_color[1], text_color[2], text_color[3]));
    title_shape->set_shape_centered(
        title_config_.TitleText(), title_frame_.getCenter(),
        title_frame_.height() * title_config_.FontSizeRatio());
  }

  void SetupCalendarLabelsShape() {
    constexpr size_t number_months = 12;
    std::array<char, kMonthNameBufferSize> buf{};
    constexpr const char* format = "%b";
    std::array<std::string, number_months> months_names;

    for (size_t index = 0; index < months_names.size(); ++index) {
      std::tm month_tm = {};
      month_tm.tm_mon = static_cast<int>(index);

      if (std::strftime(buf.data(), std::size(buf), format, &month_tm) != 0) {
        months_names.at(index) = buf.data();
      }
    }

    auto* font_shader =
        graphics_engine_->search_shader("Font Shader").value_or(nullptr);

    std::vector<rectf> x_label_frames(number_months);
    labels_font_size_ =
        font_->AdjustTextSize(rectf::from_dimension(rectf::Dimension{
                                  .width = cell_width_, .height = row_height_}),
                              "00000",
                              Font::TextScale{.height_ratio = kFontScaleMin,
                                              .width_ratio = kFontScaleMax});

    auto month_node = scene_graph_->search_node("month text").value_or(nullptr);
    if (!month_node) {
      return;
    }

    month_node->remove_children();
    for (size_t index = 0; index < number_months; ++index) {
      auto month_text_shape = std::make_shared<FontShape>(font_shader);
      month_text_shape->set_font(font_);
      auto month_text_node =
          std::make_shared<SceneNode>(months_names.at(index), month_text_shape);
      month_node->add_child(month_text_node);

      const auto float_index = static_cast<float>(index);
      const auto left = x_labels_frame_.l() + (cell_width_ * float_index);
      x_label_frames.at(index).setL(left);
      x_label_frames.at(index).setR(left + cell_width_);
      x_label_frames.at(index).setB(x_labels_frame_.b());
      x_label_frames.at(index).setT(x_labels_frame_.t());

      month_text_shape->set_shape_centered(months_names.at(index),
                                           x_label_frames.at(index).getCenter(),
                                           labels_font_size_);
    }

    auto config = shape_config_.GetShapeConfiguration("Calendar Labels");

    auto column_node =
        scene_graph_->search_node("column label area").value_or(nullptr);
    if (!column_node) {
      return;
    }
    auto column_shape =
        std::dynamic_pointer_cast<RectanglesShape>(column_node->get_shape());
    if (!column_shape) {
      return;
    }
    column_shape->set_shape(x_label_frames, config.LineWidth());
    column_shape->set_color({config.OutlineColor(), config.FillColor()});

    auto year_node = scene_graph_->search_node("year text").value_or(nullptr);
    if (!year_node) {
      return;
    }

    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    std::vector<rectf> y_labels_frames(span_years);
    year_node->remove_children();
    for (std::size_t index = 0; index < span_years; ++index) {
      const std::string current_year_text =
          std::to_string(calendar_config_.GetYear(index));

      auto year_text_shape = std::make_shared<FontShape>(font_shader);
      year_text_shape->set_font(font_);
      auto year_text_node =
          std::make_shared<SceneNode>(current_year_text, year_text_shape);
      year_node->add_child(year_text_node);

      const auto float_index = static_cast<float>(index);
      const auto bottom = y_labels_frame_.b() + (row_height_ * float_index);
      y_labels_frames.at(index).setL(y_labels_frame_.l());
      y_labels_frames.at(index).setR(y_labels_frame_.r());
      y_labels_frames.at(index).setB(bottom);
      y_labels_frames.at(index).setT(bottom + row_height_);

      year_text_shape->set_shape_centered(current_year_text,
                                          y_labels_frames.at(index).getCenter(),
                                          labels_font_size_);
    }

    auto row_node =
        scene_graph_->search_node("row label area").value_or(nullptr);
    if (!row_node) {
      return;
    }
    auto row_shape =
        std::dynamic_pointer_cast<RectanglesShape>(row_node->get_shape());
    if (!row_shape) {
      return;
    }
    row_shape->set_shape(y_labels_frames, config.LineWidth());
    row_shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupYearsShapes() {
    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    std::vector<rectf> years_cells(span_years);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = calendar_config_.GetYear(index);
      const auto number_days =
          boost::gregorian::date_period(
              boost::gregorian::date(static_cast<unsigned short>(current_year),
                                     1, 1),
              boost::gregorian::date(
                  static_cast<unsigned short>(current_year + 1), 1, 1))
              .length()
              .days();
      const float year_length = static_cast<float>(number_days) * day_width_;
      rectf year_cell = proportion_frame_layout_.GetSubFrame(index, 1);
      year_cell.setR(year_cell.l() + year_length);
      years_cells.at(index) = year_cell;
    }

    auto config = shape_config_.GetShapeConfiguration("Years Shapes");

    auto node = scene_graph_->search_node("year cells").value_or(nullptr);
    if (!node) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    if (!shape) {
      return;
    }
    shape->set_shape(years_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupMonthsShapes() {
    constexpr size_t number_months = 12;
    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    const auto store_size = number_months * span_years;
    std::vector<rectf> months_cells(store_size);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = calendar_config_.GetYear(index);
      const boost::gregorian::date first_day_of_year = boost::gregorian::date(
          static_cast<unsigned short>(current_year), 1, 1);

      for (size_t subindex = 0; subindex < number_months; ++subindex) {
        const auto current_cell =
            proportion_frame_layout_.GetSubFrame(index, 1);
        const int month_index = static_cast<int>(subindex);
        rectf month_cell;
        const auto start_offset =
            static_cast<float>(
                boost::gregorian::date_period(
                    first_day_of_year,
                    first_day_of_year + boost::gregorian::months(month_index))
                    .length()
                    .days()) *
            day_width_;
        const auto end_offset =
            static_cast<float>(boost::gregorian::date_period(
                                   first_day_of_year,
                                   first_day_of_year + boost::gregorian::months(
                                                           month_index + 1))
                                   .length()
                                   .days()) *
            day_width_;
        month_cell.setL(current_cell.l() + start_offset);
        month_cell.setR(current_cell.l() + end_offset);
        month_cell.setB(current_cell.b());
        month_cell.setT(current_cell.t());

        const auto store_index = (index * number_months) + subindex;
        months_cells.at(store_index) = month_cell;
      }
    }

    auto config = shape_config_.GetShapeConfiguration("Months Shapes");

    auto node = scene_graph_->search_node("month cells").value_or(nullptr);
    if (!node) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    if (!shape) {
      return;
    }
    shape->set_shape(months_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupDaysShapes() {
    if (!calendar_config_.IsValidSpan()) {
      return;
    }

    const auto span_days = calendar_config_.GetSpanLengthDays();
    if (span_days <= 0) {
      return;
    }

    using DurationRep = boost::gregorian::date_duration::duration_rep;
    std::int64_t days_index = 0;
    const auto number_days_cells = static_cast<size_t>(span_days);

    std::vector<rectf> days_cells0;
    std::vector<rectf> days_cells1;
    days_cells0.resize(number_days_cells);
    days_cells1.resize(number_days_cells);

    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = calendar_config_.GetYear(index);
      const std::int64_t number_days =
          boost::gregorian::date_period(
              boost::gregorian::date(static_cast<unsigned short>(current_year),
                                     1, 1),
              boost::gregorian::date(
                  static_cast<unsigned short>(current_year + 1), 1, 1))
              .length()
              .days();

      for (std::int64_t subindex = 0; subindex < number_days; ++subindex) {
        const auto float_subindex = static_cast<float>(subindex);
        const auto current_cell =
            proportion_frame_layout_.GetSubFrame(index, 1);

        const boost::gregorian::date current_date =
            calendar_config_.GetSpanLimitsDate().at(0) +
            boost::gregorian::date_duration(
                static_cast<DurationRep>(days_index));

        if (current_date.day_of_week() == boost::date_time::Sunday) {
          rectf day_cell;
          day_cell.setL(current_cell.l() + (float_subindex * day_width_));
          day_cell.setR(day_cell.l() + day_width_);
          day_cell.setB(current_cell.b());
          day_cell.setT(current_cell.t());
          days_cells1[static_cast<size_t>(days_index)] = day_cell;
        } else {
          rectf day_cell;
          day_cell.setL(current_cell.l() + (float_subindex * day_width_));
          day_cell.setR(day_cell.l() + day_width_);
          day_cell.setB(current_cell.b());
          day_cell.setT(current_cell.t());
          days_cells0[static_cast<size_t>(days_index)] = day_cell;
        }
        ++days_index;
      }
    }

    auto config = shape_config_.GetShapeConfiguration("Day Shapes");
    auto sunday_config = shape_config_.GetShapeConfiguration("Sunday Shapes");

    auto node0 = scene_graph_->search_node("day cells 0").value_or(nullptr);
    if (!node0) {
      return;
    }
    auto shape0 =
        std::dynamic_pointer_cast<RectanglesShape>(node0->get_shape());
    if (!shape0) {
      return;
    }
    shape0->set_shape(days_cells0, config.LineWidth());
    shape0->set_color({config.OutlineColor(), config.FillColor()});

    auto node1 = scene_graph_->search_node("day cells 1").value_or(nullptr);
    if (!node1) {
      return;
    }
    auto shape1 =
        std::dynamic_pointer_cast<RectanglesShape>(node1->get_shape());
    if (!shape1) {
      return;
    }
    shape1->set_shape(days_cells1, sunday_config.LineWidth());
    shape1->set_color(
        {sunday_config.OutlineColor(), sunday_config.FillColor()});
  }

  void SetupBarsShape() {
    auto node = scene_graph_->search_node("bar cells").value_or(nullptr);
    if (!node) {
      return;
    }
    node->remove_children();

    const auto number_groups = date_groups_.Items().size();
    for (size_t index = 0; index < number_groups; ++index) {
      auto child_node = std::make_shared<SceneNode>(std::string("group node ") +
                                                    std::to_string(index));
      node->add_child(child_node);
    }

    std::vector<std::vector<rectf>> bars_cells(number_groups);

    auto node_labels =
        scene_graph_->search_node("bar labels").value_or(nullptr);
    if (!node_labels) {
      return;
    }
    node_labels->remove_children();

    auto* font_shader =
        graphics_engine_->search_shader("Font Shader").value_or(nullptr);
    auto* rectangles_shader =
        graphics_engine_->search_shader("Rectangles Shader").value_or(nullptr);

    const auto number_bars = data_store_.GetNumberBars();
    for (size_t index = 0; index < number_bars; ++index) {
      const auto& bar = data_store_.GetBar(index);
      if (calendar_config_.IsInSpan(bar.GetYear())) {
        const auto current_group = static_cast<size_t>(bar.GetGroup());
        auto current_shape_config =
            shape_config_.GetDynamicConfiguration(current_group);

        const auto row = static_cast<std::size_t>(
            bar.GetYear() - calendar_config_.GetSpanLimitsYears().at(0));
        const auto current_sub_cell =
            proportion_frame_layout_.GetSubFrame(row, 1);

        rectf bar_cell;
        const auto bar_left =
            current_sub_cell.l() + (bar.GetFirstDay() * day_width_);
        const auto bar_right =
            current_sub_cell.l() + (bar.GetLastDay() * day_width_);
        bar_cell.setL(bar_left);
        bar_cell.setR(bar_right);
        bar_cell.setB(current_sub_cell.b());
        bar_cell.setT(current_sub_cell.t());

        bars_cells.at(current_group).push_back(bar_cell);

        const std::string label_text = bar.GetText();

        auto child_node = std::make_shared<SceneNode>(
            std::string("label node ") + std::to_string(index));
        node_labels->add_child(child_node);

        auto text_shape = std::make_shared<FontShape>(font_shader);
        text_shape->set_font(font_);
        child_node->set_shape(text_shape);

        auto current_text_cell = proportion_frame_layout_.GetSubFrame(row, 2);
        current_text_cell.setL(bar_cell.l());
        current_text_cell.setR(bar_cell.r());

        text_shape->set_shape_centered(label_text,
                                       current_text_cell.getCenter(),
                                       current_text_cell.height());
      }
    }

    const auto& node_children = node->get_children();
    for (size_t index = 0; index < node_children.size(); ++index) {
      auto shape = std::make_shared<RectanglesShape>(rectangles_shader);
      node_children[index]->set_shape(shape);

      auto current_shape_config = shape_config_.GetDynamicConfiguration(index);

      shape->set_shape(bars_cells.at(index), current_shape_config.LineWidth());
      shape->set_color({current_shape_config.OutlineColor(),
                        current_shape_config.FillColor()});
    }
  }

  void SetupYearsTotals() {
    auto* font_shader =
        graphics_engine_->search_shader("Font Shader").value_or(nullptr);

    auto node_cells =
        scene_graph_->search_node("year total cells").value_or(nullptr);
    if (!node_cells) {
      return;
    }

    auto node_text =
        scene_graph_->search_node("year total text").value_or(nullptr);
    if (!node_text) {
      return;
    }
    node_text->remove_children();

    const std::size_t span_years = data_store_.GetSpan();
    if (span_years == 0) {
      return;
    }

    std::vector<rectf> years_totals_cells(span_years);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year =
          data_store_.GetFirstYear() + static_cast<int>(index);
      if (calendar_config_.IsInSpan(current_year)) {
        const auto row = static_cast<std::size_t>(
            current_year - calendar_config_.GetSpanLimitsYears().at(0));
        const auto current_cell = proportion_frame_layout_.GetSubFrame(row, 0);

        rectf year_total_cell = current_cell;
        const auto year_total_width =
            static_cast<float>(data_store_.GetAnnualTotal(index)) * day_width_;
        year_total_cell.setR(current_cell.l() + year_total_width);
        years_totals_cells.at(index) = year_total_cell;

        const auto number_days =
            boost::gregorian::date_period(
                boost::gregorian::date(
                    static_cast<unsigned short>(current_year), 1, 1),
                boost::gregorian::date(
                    static_cast<unsigned short>(current_year + 1), 1, 1))
                .length()
                .days();

        const float percent =
            static_cast<float>(data_store_.GetAnnualTotal(index)) /
            static_cast<float>(number_days);

        std::ostringstream year_total_stream;
        year_total_stream << std::fixed << std::setprecision(1)
                          << percent * kPercentScale << " %";
        const auto year_total_text = year_total_stream.str();
        const auto year_total_text_width =
            font_->TextWidth(year_total_text, year_total_cell.height());

        rectf year_total_text_cell;
        year_total_text_cell.setL(year_total_cell.r() + current_cell.height());
        year_total_text_cell.setR(year_total_text_cell.l() +
                                  year_total_text_width);
        year_total_text_cell.setB(year_total_cell.b());
        year_total_text_cell.setT(year_total_cell.t());

        auto text_shape = std::make_shared<FontShape>(font_shader);
        text_shape->set_font(font_);
        text_shape->set_shape_centered(year_total_text,
                                       year_total_text_cell.getCenter(),
                                       year_total_text_cell.height());

        auto node_child = std::make_shared<SceneNode>(
            std::string("year total label ") + std::to_string(index));
        node_child->set_shape(text_shape);
        node_text->add_child(node_child);
      }
    }

    auto config = shape_config_.GetShapeConfiguration("Years Totals");

    auto shape =
        std::dynamic_pointer_cast<RectanglesShape>(node_cells->get_shape());
    if (!shape) {
      return;
    }

    shape->set_shape(years_totals_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupLegend() {
    auto* font_shader =
        graphics_engine_->search_shader("Font Shader").value_or(nullptr);
    auto* rectangles_shader =
        graphics_engine_->search_shader("Rectangles Shader").value_or(nullptr);

    // auto node_area = scene_graph->search_node("legend
    // area").value_or(nullptr);

    auto node_entries =
        scene_graph_->search_node("legend entries").value_or(nullptr);
    if (!node_entries) {
      return;
    }
    node_entries->remove_children();

    auto node_text = scene_graph_->search_node("legend text").value_or(nullptr);
    if (!node_text) {
      return;
    }
    node_text->remove_children();

    const size_t number_entrie_frames = (date_groups_.Items().size() + 1) * 2;
    std::vector<rectf> legend_entries_frames(number_entrie_frames);
    const auto entries_width =
        legend_frame_.width() / static_cast<float>(number_entrie_frames);

    for (size_t index = 0; index < number_entrie_frames; ++index) {
      const auto float_index = static_cast<float>(index);
      const auto left = legend_frame_.l() + (entries_width * float_index);
      legend_entries_frames.at(index) = legend_frame_;
      legend_entries_frames.at(index).setL(left);
      legend_entries_frames.at(index).setR(left + entries_width);
    }

    std::vector<rectf> bars_cells;

    auto print_strings = date_groups_.GetDateGroupsNames();
    print_strings.emplace_back("Annual Sums");

    std::string string_max_length;
    for (const auto& current_string : print_strings) {
      if (current_string.length() > string_max_length.length()) {
        string_max_length = current_string;
      }
    }

    const auto legend_font_size =
        font_->AdjustTextSize(legend_entries_frames.at(0), string_max_length,
                              Font::TextScale{.height_ratio = kFontScaleMin,
                                              .width_ratio = kFontScaleMax});

    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    for (size_t index = 0; index < date_groups_.Items().size(); ++index) {
      const auto label_index = index * 2;
      auto node_text_child = std::make_shared<SceneNode>(
          std::string("legend label ") + std::to_string(index));
      node_text->add_child(node_text_child);

      auto shape_text = std::make_shared<FontShape>(font_shader);
      node_text_child->set_shape(shape_text);
      shape_text->set_font(font_);
      shape_text->set_shape_centered(
          date_groups_.Items().at(index).GetName(),
          legend_entries_frames.at(label_index).getCenter(), legend_font_size);

      if (span_years > 0U) {
        const auto current_height =
            proportion_frame_layout_.GetSubFrame(0, 1).height();
        auto current_cell = legend_entries_frames.at(label_index + 1);
        const auto current_vertical_center = current_cell.getCenter()[1];
        current_cell.setB(current_vertical_center - (current_height * kHalf));
        current_cell.setT(current_vertical_center + (current_height * kHalf));
        bars_cells.emplace_back(current_cell);

        auto current_shape_config =
            shape_config_.GetDynamicConfiguration(index);

        auto node_entrie = std::make_shared<SceneNode>(
            std::string("legend bar ") + std::to_string(index));
        node_entries->add_child(node_entrie);

        auto entrie_shape =
            std::make_shared<RectanglesShape>(rectangles_shader);
        node_entrie->set_shape(entrie_shape);

        entrie_shape->set_shape(current_cell, current_shape_config.LineWidth());
        entrie_shape->set_color({current_shape_config.OutlineColor(),
                                 current_shape_config.FillColor()});
      }
    }

    {
      auto node_text_child =
          std::make_shared<SceneNode>(std::string("legend label year total"));
      node_text->add_child(node_text_child);

      auto shape_text = std::make_shared<FontShape>(font_shader);
      node_text_child->set_shape(shape_text);
      shape_text->set_font(font_);
      shape_text->set_shape_centered(
          "Annual sum",
          legend_entries_frames.at(legend_entries_frames.size() - 2)
              .getCenter(),
          legend_font_size);

      if (span_years > 0U) {
        const auto current_height =
            proportion_frame_layout_.GetSubFrame(0, 0).height();
        auto current_cell =
            legend_entries_frames.at(legend_entries_frames.size() - 1);
        const auto current_vertical_center = current_cell.getCenter()[1];
        current_cell.setB(current_vertical_center - (current_height * kHalf));
        current_cell.setT(current_vertical_center + (current_height * kHalf));
        bars_cells.emplace_back(current_cell);

        auto current_shape_config =
            shape_config_.GetShapeConfiguration("Years Totals");

        auto node_entrie =
            std::make_shared<SceneNode>(std::string("legend bar annual sum"));
        node_entries->add_child(node_entrie);

        auto entrie_shape =
            std::make_shared<RectanglesShape>(rectangles_shader);
        node_entrie->set_shape(entrie_shape);

        entrie_shape->set_shape(current_cell, current_shape_config.LineWidth());
        entrie_shape->set_color({current_shape_config.OutlineColor(),
                                 current_shape_config.FillColor()});
      }
    }
  }

  static constexpr float kZero = 0.0F;
  static constexpr float kOne = 1.0F;
  static constexpr float kHalf = 0.5F;
  static constexpr float kDefaultMargin = 5.0F;
  static constexpr float kCalendarColumns = 13.0F;
  static constexpr float kRowHeaderScale = 2.0F;
  static constexpr float kDaysPerYear = 366.0F;
  static constexpr float kFontScaleMin = 0.5F;
  static constexpr float kFontScaleMax = 0.75F;
  static constexpr float kPercentScale = 100.0F;
  static constexpr size_t kMonthNameBufferSize = 100;

  std::shared_ptr<SceneNode> scene_graph_;
  GraphicsEngine* graphics_engine_{nullptr};

  // State owned by CalendarPage, referenced here. The referenced objects stay
  // alive and stable for the builder's lifetime; only their contents change.
  const std::shared_ptr<Font>& font_;
  const rectf& page_size_;
  const rectf& page_margin_;
  const TitleConfig& title_config_;
  CalendarConfig& calendar_config_;
  const ShapeConfigSet& shape_config_;
  const DateGroups& date_groups_;
  const DateIntervalBundleBarStore& data_store_;

  // Transient layout state, recomputed on every Build().
  ProportionFrameLayout proportion_frame_layout_;
  rectf print_area_;
  rectf title_frame_;
  rectf page_margin_frame_;
  rectf calendar_frame_;
  rectf cells_frame_;
  rectf x_labels_frame_;
  rectf y_labels_frame_;
  rectf legend_frame_;
  float cell_width_{0.0F};
  float row_height_{0.0F};
  float day_width_{0.0F};
  float labels_font_size_{0.0F};
};
#endif  // CALENDAR_SCENE_BUILDER_HPP
