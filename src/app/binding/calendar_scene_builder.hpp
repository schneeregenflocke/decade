#ifndef CALENDAR_SCENE_BUILDER_HPP
#define CALENDAR_SCENE_BUILDER_HPP

#include <array>
#include <cstdint>
#include <ctime>
#include <glm/gtc/matrix_transform.hpp>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../graphics/font.hpp"
#include "../../graphics/frame_layout.hpp"
#include "../../graphics/graphics_engine.hpp"
#include "../../graphics/pick_id.hpp"
#include "../../graphics/scene_graph.hpp"
#include "../../graphics/shapes.hpp"
#include "../../packages/calendar_config.hpp"
#include "../../packages/date.hpp"
#include "../../packages/date_entry_bar_store.hpp"
#include "../../packages/date_group.hpp"
#include "../../packages/shape_configuration.hpp"
#include "../../packages/timeline_projection.hpp"
#include "../../packages/title_config.hpp"
#include "scene_snapshot.hpp"
#include "scene_snapshot_builder.hpp"

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
                       const DateEntryBarStore& data_store_in)
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
    graphics_engine_->SetSceneGraph(scene_graph_);
    auto* simple_shader =
        graphics_engine_->SearchShader("Simple Shader").value_or(nullptr);
    rectangles_shader_ =
        graphics_engine_->SearchShader("Rectangles Shader").value_or(nullptr);
    font_shader_ =
        graphics_engine_->SearchShader("Font Shader").value_or(nullptr);
    auto* rectangles_shader = rectangles_shader_;
    auto* font_shader = font_shader_;

    // The scene skeleton is built once here; each named node is kept as a
    // member so Build()/Setup* can reach it directly.
    auto page_shape = std::make_shared<QuadrilateralShape>(simple_shader);
    page_node_ = std::make_shared<SceneNode>("page", page_shape);
    scene_graph_->AddChild(page_node_);

    auto print_area_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    print_area_node_ =
        std::make_shared<SceneNode>("print area", print_area_shape);
    page_node_->AddChild(print_area_node_);

    auto title_area_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    title_area_node_ =
        std::make_shared<SceneNode>("title area", title_area_shape);
    print_area_node_->AddChild(title_area_node_);

    auto row_labels_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    row_labels_node_ =
        std::make_shared<SceneNode>("row label area", row_labels_shape);
    print_area_node_->AddChild(row_labels_node_);

    auto column_labels_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    column_labels_node_ =
        std::make_shared<SceneNode>("column label area", column_labels_shape);
    print_area_node_->AddChild(column_labels_node_);

    auto years_cells_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    years_cells_node_ =
        std::make_shared<SceneNode>("year cells", years_cells_shape);
    print_area_node_->AddChild(years_cells_node_);

    auto months_cells_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    months_cells_node_ =
        std::make_shared<SceneNode>("month cells", months_cells_shape);
    print_area_node_->AddChild(months_cells_node_);

    auto days_cells0_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    days_cells0_node_ =
        std::make_shared<SceneNode>("day cells 0", days_cells0_shape);
    print_area_node_->AddChild(days_cells0_node_);

    auto days_cells1_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    days_cells1_node_ =
        std::make_shared<SceneNode>("day cells 1", days_cells1_shape);
    print_area_node_->AddChild(days_cells1_node_);

    bars_cells_node_ = std::make_shared<SceneNode>("bar cells");
    print_area_node_->AddChild(bars_cells_node_);

    auto years_totals_shape =
        std::make_shared<RectanglesShape>(rectangles_shader);
    years_totals_node_ =
        std::make_shared<SceneNode>("year total cells", years_totals_shape);
    print_area_node_->AddChild(years_totals_node_);

    years_totals_text_node_ = std::make_shared<SceneNode>("year total text");
    print_area_node_->AddChild(years_totals_text_node_);

    auto legend_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto legend_shape_node =
        std::make_shared<SceneNode>("legend area", legend_shape);
    print_area_node_->AddChild(legend_shape_node);

    legend_entries_node_ = std::make_shared<SceneNode>("legend entries");
    print_area_node_->AddChild(legend_entries_node_);

    legend_text_node_ = std::make_shared<SceneNode>("legend text");
    print_area_node_->AddChild(legend_text_node_);

    auto title_font_shape = std::make_shared<FontShape>(font_shader);
    title_font_shape->SetFont(font_);
    title_font_node_ =
        std::make_shared<SceneNode>("title text", title_font_shape);
    print_area_node_->AddChild(title_font_node_);

    month_text_node_ = std::make_shared<SceneNode>("month text");
    print_area_node_->AddChild(month_text_node_);

    year_text_node_ = std::make_shared<SceneNode>("year text");
    print_area_node_->AddChild(year_text_node_);

    bar_labels_node_ = std::make_shared<SceneNode>("bar labels");
    print_area_node_->AddChild(bar_labels_node_);

    // Painter layers for the fixed shape-bearing nodes. Container nodes (bar
    // cells, the text groups, …) carry no shape; their dynamic children get
    // their layer where they are created in the Setup* methods.
    page_node_->SetDrawLayer(kLayerPage);
    print_area_node_->SetDrawLayer(kLayerFrame);
    title_area_node_->SetDrawLayer(kLayerFrame);
    legend_shape_node->SetDrawLayer(kLayerFrame);
    row_labels_node_->SetDrawLayer(kLayerGrid);
    column_labels_node_->SetDrawLayer(kLayerGrid);
    years_cells_node_->SetDrawLayer(kLayerGrid);
    months_cells_node_->SetDrawLayer(kLayerGrid);
    days_cells0_node_->SetDrawLayer(kLayerGrid);
    days_cells1_node_->SetDrawLayer(kLayerGrid);
    years_totals_node_->SetDrawLayer(kLayerBars);
    title_font_node_->SetDrawLayer(kLayerText);
  }

  void Build() {
    auto shape =
        std::dynamic_pointer_cast<QuadrilateralShape>(page_node_->GetShape());
    if (!shape) {
      return;
    }
    shape->SetShape(page_size_);
    shape->SetColor(glm::vec4(kOne, kOne, kOne, kOne));

    // Position the whole calendar relative to the print-area node: the node
    // carries the print area's offset within the page, and every descendant is
    // computed in print-area-local coordinates (origin at the print area's
    // bottom-left). The page rectangle itself stays in absolute page space on
    // the untransformed page node above.
    print_area_ = page_size_.reduce(page_margin_);
    print_area_origin_ = print_area_.getLB();
    print_area_node_->SetModelMatrix(
        glm::translate(glm::mat4(1.0F), print_area_origin_));
    print_area_ =
        print_area_.shift(-print_area_origin_.x, -print_area_origin_.y);

    title_frame_ = print_area_;
    title_frame_.setB(title_frame_.t() - title_config_.FrameHeight());

    page_margin_frame_ = print_area_;
    page_margin_frame_.setT(title_frame_.b());

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

  // Plain, GL-free mirror of the current scene-graph hierarchy for the
  // presentation layer (the scene-tree widget). Rebuilt on demand from the
  // live graph after Build().
  [[nodiscard]] SceneNodeSnapshot SceneSnapshot() const {
    return BuildSceneSnapshot(*scene_graph_);
  }

  // Page-space rectangles of the pickable bars, produced by the last Build().
  // Handed to the picking layer; Bullet-free.
  [[nodiscard]] const std::vector<PickBox>& BarPickBoxes() const {
    return bar_pick_boxes_;
  }

  // Highlights the hovered bar (and restores the previously hovered one) by
  // recolouring its shape in place — no scene rebuild. A null value clears the
  // highlight.
  void SetHoveredBar(const std::optional<PickId>& hovered) {
    if (hovered == hovered_bar_) {
      return;
    }
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/false);
    }
    hovered_bar_ = hovered;
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
    }
  }

 private:
  // Recolours one bar's shape: highlighted bars get a distinct outline, normal
  // bars are restored to their group's configured colours. Fill is left as
  // configured so the hover reads as an outline accent.
  void ApplyBarColor(std::size_t bar_index, bool highlighted) {
    const auto iterator = bar_nodes_.find(bar_index);
    if (iterator == bar_nodes_.end() ||
        bar_index >= data_store_.GetNumberBars()) {
      return;
    }
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(
        iterator->second->GetShape());
    if (!shape) {
      return;
    }
    const auto group =
        static_cast<std::size_t>(data_store_.GetBar(bar_index).GetGroup());
    const auto config = shape_config_.GetDynamicConfiguration(group);
    if (highlighted) {
      const glm::vec4 hover_outline(kOne, kHoverOutlineGreen, kZero, kOne);
      shape->SetColor({hover_outline, config.FillColor()});
    } else {
      shape->SetColor({config.OutlineColor(), config.FillColor()});
    }
  }

  // Fills the RectanglesShape carried by `node` with the given rectangle(s)
  // (a single rectf or a vector of them — SetShape is overloaded) and the
  // outline/fill colours of `config`. The cast cannot fail for the fixed
  // skeleton nodes, which are all created with a RectanglesShape.
  template <typename Shapes>
  static void FillRectangles(const std::shared_ptr<SceneNode>& node,
                             const Shapes& shapes,
                             const ShapeConfiguration& config) {
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->GetShape());
    if (!shape) {
      return;
    }
    shape->SetShape(shapes, config.LineWidth());
    shape->SetColor({config.OutlineColor(), config.FillColor()});
  }

  // Creates a centered text node named `name` under `parent`: a FontShape on
  // the text layer rendering `text` centered at `center` with font height
  // `size`. Concentrates the repeated "make FontShape, SetFont, centre, attach
  // on the text layer" sequence used by every label group.
  void AddCenteredText(const std::shared_ptr<SceneNode>& parent,
                       const std::string& name, const std::string& text,
                       const glm::vec3& center, float size) {
    auto shape = std::make_shared<FontShape>(font_shader_);
    shape->SetFont(font_);
    shape->SetShapeCentered(text, center, size);
    auto node = std::make_shared<SceneNode>(name, shape);
    node->SetDrawLayer(kLayerText);
    parent->AddChild(node);
  }

  void SetupPrintAreaShape() {
    FillRectangles(print_area_node_, print_area_,
                   shape_config_.GetShapeConfiguration("Page Margin"));
  }

  void SetupTitleShape() {
    FillRectangles(title_area_node_, title_frame_,
                   shape_config_.GetShapeConfiguration("Title Frame"));

    auto title_shape =
        std::dynamic_pointer_cast<FontShape>(title_font_node_->GetShape());
    if (!title_shape) {
      return;
    }
    title_shape->SetFont(font_);
    const std::array<float, 4>& text_color = title_config_.TextColor();
    title_shape->SetColor(
        glm::vec4(text_color[0], text_color[1], text_color[2], text_color[3]));
    title_shape->SetShapeCentered(
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

    std::vector<rectf> x_label_frames(number_months);
    labels_font_size_ =
        font_->AdjustTextSize(rectf::from_dimension(rectf::Dimension{
                                  .width = cell_width_, .height = row_height_}),
                              "00000",
                              Font::TextScale{.height_ratio = kFontScaleMin,
                                              .width_ratio = kFontScaleMax});

    auto& month_node = month_text_node_;

    month_node->RemoveChildren();
    for (size_t index = 0; index < number_months; ++index) {
      const auto float_index = static_cast<float>(index);
      const auto left = x_labels_frame_.l() + (cell_width_ * float_index);
      x_label_frames.at(index).setL(left);
      x_label_frames.at(index).setR(left + cell_width_);
      x_label_frames.at(index).setB(x_labels_frame_.b());
      x_label_frames.at(index).setT(x_labels_frame_.t());

      AddCenteredText(month_node, months_names.at(index),
                      months_names.at(index),
                      x_label_frames.at(index).getCenter(), labels_font_size_);
    }

    const auto config = shape_config_.GetShapeConfiguration("Calendar Labels");
    FillRectangles(column_labels_node_, x_label_frames, config);

    auto& year_node = year_text_node_;

    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    const TimelineProjection projection(calendar_config_);
    std::vector<rectf> y_labels_frames(span_years);
    year_node->RemoveChildren();
    for (std::size_t index = 0; index < span_years; ++index) {
      const std::string current_year_text =
          std::to_string(projection.YearForRow(index));

      const auto float_index = static_cast<float>(index);
      const auto bottom = y_labels_frame_.b() + (row_height_ * float_index);
      y_labels_frames.at(index).setL(y_labels_frame_.l());
      y_labels_frames.at(index).setR(y_labels_frame_.r());
      y_labels_frames.at(index).setB(bottom);
      y_labels_frames.at(index).setT(bottom + row_height_);

      AddCenteredText(year_node, current_year_text, current_year_text,
                      y_labels_frames.at(index).getCenter(), labels_font_size_);
    }

    FillRectangles(row_labels_node_, y_labels_frames, config);
  }

  void SetupYearsShapes() {
    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    const TimelineProjection projection(calendar_config_);
    std::vector<rectf> years_cells(span_years);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = projection.YearForRow(index);
      const auto number_days = DaysInYear(current_year);
      const float year_length = static_cast<float>(number_days) * day_width_;
      rectf year_cell = proportion_frame_layout_.GetSubFrame(index, 1);
      year_cell.setR(year_cell.l() + year_length);
      years_cells.at(index) = year_cell;
    }

    FillRectangles(years_cells_node_, years_cells,
                   shape_config_.GetShapeConfiguration("Years Shapes"));
  }

  void SetupMonthsShapes() {
    constexpr size_t number_months = 12;
    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    if (span_years == 0) {
      return;
    }

    const auto store_size = number_months * span_years;
    const TimelineProjection projection(calendar_config_);
    std::vector<rectf> months_cells(store_size);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = projection.YearForRow(index);
      const Date first_day_of_year = Date::FromYmd(current_year, 1, 1);

      for (size_t subindex = 0; subindex < number_months; ++subindex) {
        const auto current_cell =
            proportion_frame_layout_.GetSubFrame(index, 1);
        const int month_index = static_cast<int>(subindex);
        rectf month_cell;
        const auto start_offset =
            static_cast<float>(Date::DaysBetween(
                first_day_of_year, first_day_of_year.AddMonths(month_index))) *
            day_width_;
        const auto end_offset =
            static_cast<float>(Date::DaysBetween(
                first_day_of_year,
                first_day_of_year.AddMonths(month_index + 1))) *
            day_width_;
        month_cell.setL(current_cell.l() + start_offset);
        month_cell.setR(current_cell.l() + end_offset);
        month_cell.setB(current_cell.b());
        month_cell.setT(current_cell.t());

        const auto store_index = (index * number_months) + subindex;
        months_cells.at(store_index) = month_cell;
      }
    }

    FillRectangles(months_cells_node_, months_cells,
                   shape_config_.GetShapeConfiguration("Months Shapes"));
  }

  void SetupDaysShapes() {
    if (!calendar_config_.IsValidSpan()) {
      return;
    }

    const auto span_days = calendar_config_.GetSpanLengthDays();
    if (span_days <= 0) {
      return;
    }

    std::int64_t days_index = 0;
    const auto number_days_cells = static_cast<size_t>(span_days);

    std::vector<rectf> days_cells0;
    std::vector<rectf> days_cells1;
    days_cells0.resize(number_days_cells);
    days_cells1.resize(number_days_cells);

    const std::size_t span_years = calendar_config_.GetSpanLengthYears();
    const TimelineProjection projection(calendar_config_);
    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year = projection.YearForRow(index);
      const std::int64_t number_days = DaysInYear(current_year);

      for (std::int64_t subindex = 0; subindex < number_days; ++subindex) {
        const auto float_subindex = static_cast<float>(subindex);
        const auto current_cell =
            proportion_frame_layout_.GetSubFrame(index, 1);

        const Date current_date =
            calendar_config_.GetSpanLimitsDate().at(0).AddDays(
                static_cast<int>(days_index));

        if (current_date.DayOfWeek() == Weekday::kSunday) {
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

    FillRectangles(days_cells0_node_, days_cells0,
                   shape_config_.GetShapeConfiguration("Day Shapes"));
    FillRectangles(days_cells1_node_, days_cells1,
                   shape_config_.GetShapeConfiguration("Sunday Shapes"));
  }

  void SetupBarsShape() {
    auto& node = bars_cells_node_;
    node->RemoveChildren();

    const auto number_groups = date_groups_.Items().size();
    std::vector<std::shared_ptr<SceneNode>> group_nodes;
    group_nodes.reserve(number_groups);
    for (size_t index = 0; index < number_groups; ++index) {
      auto group_node = std::make_shared<SceneNode>(std::string("group node ") +
                                                    std::to_string(index));
      node->AddChild(group_node);
      group_nodes.push_back(group_node);
    }

    auto& node_labels = bar_labels_node_;
    node_labels->RemoveChildren();

    bar_pick_boxes_.clear();
    bar_nodes_.clear();

    const TimelineProjection projection(calendar_config_);
    const auto number_bars = data_store_.GetNumberBars();
    for (size_t index = 0; index < number_bars; ++index) {
      const auto& bar = data_store_.GetBar(index);
      if (!calendar_config_.IsInSpan(bar.GetYear())) {
        continue;
      }
      const auto current_group = static_cast<size_t>(bar.GetGroup());
      auto current_shape_config =
          shape_config_.GetDynamicConfiguration(current_group);

      const auto row = projection.RowForYear(bar.GetYear());
      const auto current_sub_cell =
          proportion_frame_layout_.GetSubFrame(row, 1);

      const auto bar_left =
          current_sub_cell.l() + (bar.GetFirstDay() * day_width_);
      const auto bar_width =
          (bar.GetLastDay() - bar.GetFirstDay()) * day_width_;
      const auto bar_height = current_sub_cell.height();

      // Each bar is its own node: the position lives in the node transform
      // (ready for dragging/animating), the size lives in the shape geometry.
      // A pure translation keeps the outline width constant, which a scale
      // matrix would distort. The bar's world rect is therefore unchanged.
      auto bar_node = std::make_shared<SceneNode>(std::string("bar ") +
                                                  std::to_string(index));
      bar_node->SetModelMatrix(glm::translate(
          glm::mat4(1.0F), glm::vec3(bar_left, current_sub_cell.b(), kZero)));

      // Page-space box for hit-testing. The node's world position is
      // print_area_origin_ + (bar_left, sub_cell.b()), so the page-space rect
      // is the local bar rect shifted by that origin.
      const PickId pick_id{.kind = PickId::Kind::kBar, .index = index};
      bar_pick_boxes_.push_back(PickBox{
          .id = pick_id,
          .rect =
              rectf(bar_left + print_area_origin_.x,
                    bar_left + bar_width + print_area_origin_.x,
                    current_sub_cell.b() + print_area_origin_.y,
                    current_sub_cell.b() + bar_height + print_area_origin_.y)});

      auto bar_shape = std::make_shared<RectanglesShape>(rectangles_shader_);
      bar_shape->SetShape(rectf(kZero, bar_width, kZero, bar_height),
                          current_shape_config.LineWidth());
      bar_shape->SetColor({current_shape_config.OutlineColor(),
                           current_shape_config.FillColor()});
      bar_node->SetShape(bar_shape);
      bar_node->SetDrawLayer(kLayerBars);
      group_nodes.at(current_group)->AddChild(bar_node);
      bar_nodes_.emplace(index, bar_node);

      auto current_text_cell = proportion_frame_layout_.GetSubFrame(row, 2);
      current_text_cell.setL(bar_left);
      current_text_cell.setR(bar_left + bar_width);

      AddCenteredText(node_labels,
                      std::string("label node ") + std::to_string(index),
                      bar.GetText(), current_text_cell.getCenter(),
                      current_text_cell.height());
    }

    // Re-apply the hover highlight to the freshly built node, if still hovered.
    if (hovered_bar_.has_value()) {
      ApplyBarColor(hovered_bar_->index, /*highlighted=*/true);
    }
  }

  void SetupYearsTotals() {
    auto& node_cells = years_totals_node_;
    auto& node_text = years_totals_text_node_;
    node_text->RemoveChildren();

    const std::size_t span_years = data_store_.GetSpan();
    if (span_years == 0) {
      return;
    }

    const TimelineProjection projection(calendar_config_);
    std::vector<rectf> years_totals_cells(span_years);

    for (std::size_t index = 0; index < span_years; ++index) {
      const int current_year =
          data_store_.GetFirstYear() + static_cast<int>(index);
      if (calendar_config_.IsInSpan(current_year)) {
        const auto row = projection.RowForYear(current_year);
        const auto current_cell = proportion_frame_layout_.GetSubFrame(row, 0);

        rectf year_total_cell = current_cell;
        const auto year_total_width =
            static_cast<float>(data_store_.GetAnnualTotal(index)) * day_width_;
        year_total_cell.setR(current_cell.l() + year_total_width);
        years_totals_cells.at(index) = year_total_cell;

        const auto number_days = DaysInYear(current_year);

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

        AddCenteredText(
            node_text, std::string("year total label ") + std::to_string(index),
            year_total_text, year_total_text_cell.getCenter(),
            year_total_text_cell.height());
      }
    }

    FillRectangles(node_cells, years_totals_cells,
                   shape_config_.GetShapeConfiguration("Years Totals"));
  }

  void SetupLegend() {
    auto& node_entries = legend_entries_node_;
    node_entries->RemoveChildren();

    auto& node_text = legend_text_node_;
    node_text->RemoveChildren();

    const size_t number_entry_frames = (date_groups_.Items().size() + 1) * 2;
    std::vector<rectf> legend_entries_frames(number_entry_frames);
    const auto entries_width =
        legend_frame_.width() / static_cast<float>(number_entry_frames);

    for (size_t index = 0; index < number_entry_frames; ++index) {
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
      AddCenteredText(
          node_text, std::string("legend label ") + std::to_string(index),
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

        auto node_entry = std::make_shared<SceneNode>(
            std::string("legend bar ") + std::to_string(index));
        node_entry->SetDrawLayer(kLayerBars);
        node_entries->AddChild(node_entry);

        auto entry_shape =
            std::make_shared<RectanglesShape>(rectangles_shader_);
        node_entry->SetShape(entry_shape);

        entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
        entry_shape->SetColor({current_shape_config.OutlineColor(),
                               current_shape_config.FillColor()});
      }
    }

    {
      AddCenteredText(node_text, std::string("legend label year total"),
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

        auto current_shape_config = shape_config_.GetShapeConfiguration(
            ShapeConfigSet::AnnualSumConfigurationName());

        auto node_entry =
            std::make_shared<SceneNode>(std::string("legend bar annual sum"));
        node_entry->SetDrawLayer(kLayerBars);
        node_entries->AddChild(node_entry);

        auto entry_shape =
            std::make_shared<RectanglesShape>(rectangles_shader_);
        node_entry->SetShape(entry_shape);

        entry_shape->SetShape(current_cell, current_shape_config.LineWidth());
        entry_shape->SetColor({current_shape_config.OutlineColor(),
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
  static constexpr float kHoverOutlineGreen = 0.55F;

  // Painter draw layers (lower = further back). The bars sit above the grid
  // background so the day/sunday/month cells no longer cover them; text sits on
  // top of everything.
  static constexpr int kLayerPage = 0;
  static constexpr int kLayerFrame = 10;
  static constexpr int kLayerGrid = 20;
  static constexpr int kLayerBars = 30;
  static constexpr int kLayerText = 40;

  std::shared_ptr<SceneNode> scene_graph_;
  GraphicsEngine* graphics_engine_{nullptr};

  // Cached shader handles, looked up once in the constructor. The shaders live
  // in the GraphicsEngine for the builder's whole lifetime, so the Setup*
  // methods reach for these instead of repeating the name lookup.
  Shader* rectangles_shader_{nullptr};
  Shader* font_shader_{nullptr};

  // Stable handles to the fixed scene-skeleton nodes, created once in the
  // constructor and reached directly by Build() and the Setup* methods.
  std::shared_ptr<SceneNode> page_node_;
  std::shared_ptr<SceneNode> print_area_node_;
  std::shared_ptr<SceneNode> title_area_node_;
  std::shared_ptr<SceneNode> title_font_node_;
  std::shared_ptr<SceneNode> row_labels_node_;
  std::shared_ptr<SceneNode> column_labels_node_;
  std::shared_ptr<SceneNode> years_cells_node_;
  std::shared_ptr<SceneNode> months_cells_node_;
  std::shared_ptr<SceneNode> days_cells0_node_;
  std::shared_ptr<SceneNode> days_cells1_node_;
  std::shared_ptr<SceneNode> bars_cells_node_;
  std::shared_ptr<SceneNode> years_totals_node_;
  std::shared_ptr<SceneNode> years_totals_text_node_;
  std::shared_ptr<SceneNode> legend_entries_node_;
  std::shared_ptr<SceneNode> legend_text_node_;
  std::shared_ptr<SceneNode> month_text_node_;
  std::shared_ptr<SceneNode> year_text_node_;
  std::shared_ptr<SceneNode> bar_labels_node_;

  // State owned by CalendarPage, referenced here. The referenced objects stay
  // alive and stable for the builder's lifetime; only their contents change.
  const std::shared_ptr<Font>& font_;
  const rectf& page_size_;
  const rectf& page_margin_;
  const TitleConfig& title_config_;
  CalendarConfig& calendar_config_;
  const ShapeConfigSet& shape_config_;
  const DateGroups& date_groups_;
  const DateEntryBarStore& data_store_;

  // Bar nodes by bar index, rebuilt each Build() — used to recolour a bar on
  // hover. The hovered bar persists across rebuilds so the highlight is
  // re-applied to the fresh node.
  std::unordered_map<std::size_t, std::shared_ptr<SceneNode>> bar_nodes_;
  std::optional<PickId> hovered_bar_;

  // Transient layout state, recomputed on every Build().
  ProportionFrameLayout proportion_frame_layout_;
  glm::vec3 print_area_origin_{0.0F};
  std::vector<PickBox> bar_pick_boxes_;
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
