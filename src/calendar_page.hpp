/*
Dekade
Copyright (c) 2019-2024 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "frame_layout.hpp"
#include "graphics/font.hpp"
#include "graphics/scene_graph.hpp"
#include "graphics/shapes.hpp"
#include "gui/opengl_panel.hpp"
#include "packages/calendar_config.hpp"
#include "packages/date_store.hpp"
#include "packages/group_store.hpp"
#include "packages/page_config.hpp"
#include "packages/shape_config.hpp"
#include "packages/title_config.hpp"
#include <array>
#include <ctime>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

class CalendarPage {
public:
  CalendarPage(GLCanvas *gl_canvas, const std::string &font_filepath) : gl_canvas(gl_canvas)
  {
    scene_graph = std::make_shared<SceneNode>("root");

    graphics_engine = gl_canvas->GraphicsEnginePtr();
    graphics_engine->set_scene_graph(scene_graph);

    font = std::make_shared<Font>(font_filepath);

    auto simple_shader = graphics_engine->search_shader("Basic Shader");
    auto rectangles_shader = graphics_engine->search_shader("Rectangles Shader");
    auto font_shader = graphics_engine->search_shader("Font Shader");

    auto page_shape = std::make_shared<QuadrilateralShape>(simple_shader);
    auto page_node = std::make_shared<SceneNode>("page", page_shape);
    scene_graph->add_child(page_node);

    auto print_area_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto print_area_node = std::make_shared<SceneNode>("print area", print_area_shape);
    page_node->add_child(print_area_node);

    auto title_area_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto title_area_node = std::make_shared<SceneNode>("title area", title_area_shape);
    print_area_node->add_child(title_area_node);

    auto row_labels_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto row_labels_node = std::make_shared<SceneNode>("row label area", row_labels_shape);
    print_area_node->add_child(row_labels_node);

    auto column_labels_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto column_labels_node = std::make_shared<SceneNode>("column label area", column_labels_shape);
    print_area_node->add_child(column_labels_node);

    auto years_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto years_cells_node = std::make_shared<SceneNode>("year cells", years_cells_shape);
    print_area_node->add_child(years_cells_node);

    auto months_cells_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto months_cells_node = std::make_shared<SceneNode>("month cells", months_cells_shape);
    print_area_node->add_child(months_cells_node);

    auto days_cells0_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto days_cells0_node = std::make_shared<SceneNode>("day cells 0", days_cells0_shape);
    print_area_node->add_child(days_cells0_node);

    auto days_cells1_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto days_cells1_node = std::make_shared<SceneNode>("day cells 1", days_cells1_shape);
    print_area_node->add_child(days_cells1_node);

    auto bars_cells_node = std::make_shared<SceneNode>("bar cells");
    print_area_node->add_child(bars_cells_node);

    auto years_totals_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto years_totals_node = std::make_shared<SceneNode>("year total cells", years_totals_shape);
    print_area_node->add_child(years_totals_node);

    auto years_totals_text_node = std::make_shared<SceneNode>("year total text");
    print_area_node->add_child(years_totals_text_node);

    auto legend_shape = std::make_shared<RectanglesShape>(rectangles_shader);
    auto legend_shape_node = std::make_shared<SceneNode>("legend area", legend_shape);
    print_area_node->add_child(legend_shape_node);

    auto legend_entries_node = std::make_shared<SceneNode>("legend entries");
    print_area_node->add_child(legend_entries_node);

    auto legend_text_node = std::make_shared<SceneNode>("legend text");
    print_area_node->add_child(legend_text_node);

    auto title_font_shape = std::make_shared<FontShape>(font_shader);
    title_font_shape->set_font(font);
    auto title_font_node = std::make_shared<SceneNode>("title text", title_font_shape);
    print_area_node->add_child(title_font_node);

    auto month_text_node = std::make_shared<SceneNode>("month text");
    print_area_node->add_child(month_text_node);

    auto year_text_node = std::make_shared<SceneNode>("year text");
    print_area_node->add_child(year_text_node);

    auto bar_labels_node = std::make_shared<SceneNode>("bar labels");
    print_area_node->add_child(bar_labels_node);
  }

  void ReceiveDateGroups(const std::vector<DateGroup> &date_groups)
  {
    date_group_store.ReceiveDateGroups(date_groups);
    data_store.ReceiveDateGroups(date_groups);
    Update();
  }

  void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle> &date_interval_bundles)
  {
    data_store.ReceiveDateIntervalBundles(date_interval_bundles);
    Update();
  }

  void ReceivePageSetup(const PageSetupConfig &page_setup_config)
  {
    this->page_size = rectf::from_dimension(page_setup_config.size[0], page_setup_config.size[1]);
    this->page_margin = rectf(page_setup_config.margins[0], page_setup_config.margins[1],
                              page_setup_config.margins[2], page_setup_config.margins[3]);
    Update();
  }

  void ReceiveFont(const std::string &font_filepath)
  {
    font = std::make_shared<Font>(font_filepath);
    Update();
  }

  void ReceiveTitleConfig(const TitleConfig &title_config)
  {
    this->title_config = title_config;
    Update();
  }

  void ReceiveCalendarConfig(const CalendarConfigStorage &calendar_config)
  {
    this->calendar_config = calendar_config;
    Update();
  }

  void
  ReceiveShapeConfigurationStorage(const ShapeConfigurationStorage &shape_configuration_storage)
  {
    this->shape_configuration_storage = shape_configuration_storage;
    Update();
  }

  void Update()
  {
    auto node = scene_graph->search_node("page");
    auto shape = std::dynamic_pointer_cast<QuadrilateralShape>(node->get_shape());
    shape->set_shape(page_size);
    shape->set_color(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    print_area = page_size.reduce(page_margin);

    title_frame = print_area;
    title_frame.setB(title_frame.t() - title_config.frame_height);

    page_margin_frame = print_area;
    page_margin_frame.setT(title_frame.b());
    // const rectf page_margin_frame_margin(0.0f, 0.0f, 0.0f, 0.f);
    // page_margin_frame = page_margin_frame.reduce(page_margin_frame_margin);

    calendar_frame = page_margin_frame;
    const rectf calendar_frame_margin = rectf(0.0f, 5.0f, 0.0f, 0.0f);
    calendar_frame = calendar_frame.reduce(calendar_frame_margin);

    if (calendar_config.auto_calendar_span == true && data_store.is_empty() == false) {
      calendar_config.SetSpan(data_store.GetFirstYear(), data_store.GetLastYear());
    }

    const size_t additional_rows = 2;
    size_t number_rows = additional_rows + calendar_config.GetSpanLengthYears();

    cell_width = calendar_frame.width() / 13.0f;
    row_height = calendar_frame.height() / static_cast<float>(number_rows);

    const rectf cells_frame_margin(cell_width, 0.f, row_height * 2.0f, 0.f);
    cells_frame = calendar_frame.reduce(cells_frame_margin);

    proportion_frame_layout.SetupRowFrames(cells_frame, calendar_config.GetSpanLengthYears());
    proportion_frame_layout.SetupSubFrames(calendar_config.spacing_proportions);

    day_width = cells_frame.width() / 366.f;

    x_labels_frame =
        calendar_frame.reduce(rectf(cell_width, 0.f, row_height, cells_frame.height()));
    y_labels_frame = calendar_frame.reduce(rectf(0.f, cells_frame.width(), row_height * 2.f, 0.f));
    legend_frame =
        calendar_frame.reduce(rectf(cell_width, 0.f, 0.f, cells_frame.height() + row_height));

    SetupPrintAreaShape();
    SetupTitleShape();
    SetupCalendarLabelsShape();
    SetupDaysShapes();
    SetupMonthsShapes();
    SetupYearsShapes();
    SetupBarsShape();
    SetupYearsTotals();
    SetupLegend();

    gl_canvas->RefreshMVP();
  }

  void SetupPrintAreaShape()
  {
    auto config = shape_configuration_storage.GetShapeConfiguration("Page Margin");

    auto node = scene_graph->search_node("print area");
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());

    shape->set_shape(print_area, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupTitleShape()
  {
    auto config = shape_configuration_storage.GetShapeConfiguration("Title Frame");

    auto node = scene_graph->search_node("title area");
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());

    shape->set_shape(title_frame, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});

    auto title_node = scene_graph->search_node("title text");
    auto title_shape = std::dynamic_pointer_cast<FontShape>(title_node->get_shape());
    title_shape->set_font(font);
    title_shape->set_shape_centered(title_config.title_text, title_frame.getCenter(),
                                    title_frame.height() * title_config.font_size_ratio);
  }

  void SetupCalendarLabelsShape()
  {
    constexpr size_t number_months = 12;
    std::array<char, 100> buf{};
    constexpr std::string_view format = "%b";
    std::array<std::string, number_months> months_names;

    for (size_t index = 0; index < months_names.size(); ++index) {
      std::tm month_tm = {};
      month_tm.tm_mon = static_cast<int>(index);

      if (std::strftime(buf.data(), std::size(buf), format.data(), &month_tm)) {
        months_names[index] = buf.data();
      }
    }

    auto font_shader = graphics_engine->search_shader("Font Shader");

    std::vector<rectf> x_label_frames(number_months);
    labels_font_size =
        font->AdjustTextSize(rectf::from_dimension(cell_width, row_height), "00000", 0.5f, 0.75f);

    auto month_node = scene_graph->search_node("month text");

    month_node->remove_children();
    for (size_t index = 0; index < number_months; ++index) {
      auto month_text_shape = std::make_shared<FontShape>(font_shader);
      month_text_shape->set_font(font);
      auto month_text_node = std::make_shared<SceneNode>(months_names[index], month_text_shape);
      month_node->add_child(month_text_node);

      float float_index = static_cast<float>(index);
      x_label_frames[index].setL(x_labels_frame.l() + cell_width * float_index);
      x_label_frames[index].setR(x_labels_frame.l() + cell_width * float_index + cell_width);
      x_label_frames[index].setB(x_labels_frame.b());
      x_label_frames[index].setT(x_labels_frame.t());

      month_text_shape->set_shape_centered(months_names[index], x_label_frames[index].getCenter(),
                                           labels_font_size);
    }

    auto config = shape_configuration_storage.GetShapeConfiguration("Calendar Labels");

    auto column_node = scene_graph->search_node("column label area");
    auto column_shape = std::dynamic_pointer_cast<RectanglesShape>(column_node->get_shape());
    column_shape->set_shape(x_label_frames, config.LineWidth());
    column_shape->set_color({config.OutlineColor(), config.FillColor()});

    auto year_node = scene_graph->search_node("year text");

    std::vector<rectf> y_labels_frames(calendar_config.GetSpanLengthYears());
    year_node->remove_children();
    for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index) {
      std::string current_year_text = std::to_string(calendar_config.GetYear(index));

      auto year_text_shape = std::make_shared<FontShape>(font_shader);
      year_text_shape->set_font(font);
      auto year_text_node = std::make_shared<SceneNode>(current_year_text, year_text_shape);
      year_node->add_child(year_text_node);

      float float_index = static_cast<float>(index);
      y_labels_frames[index].setL(y_labels_frame.l());
      y_labels_frames[index].setR(y_labels_frame.r());
      y_labels_frames[index].setB(y_labels_frame.b() + row_height * float_index);
      y_labels_frames[index].setT(y_labels_frame.b() + row_height * float_index + row_height);

      year_text_shape->set_shape_centered(current_year_text, y_labels_frames[index].getCenter(),
                                          labels_font_size);
    }

    auto row_node = scene_graph->search_node("row label area");
    auto row_shape = std::dynamic_pointer_cast<RectanglesShape>(column_node->get_shape());
    row_shape->set_shape(y_labels_frames, config.LineWidth());
    row_shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupYearsShapes()
  {
    std::vector<rectf> years_cells;
    years_cells.resize(calendar_config.GetSpanLengthYears());

    for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index) {
      int current_year = calendar_config.GetYear(index);
      int number_days =
          boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1),
                                        boost::gregorian::date(current_year + 1, 1, 1))
              .length()
              .days();
      float year_lenght = static_cast<float>(number_days) * day_width;
      proportion_frame_layout.GetSubFrame(index, 1);
      rectf year_cell = proportion_frame_layout.GetSubFrame(index, 1);
      year_cell.setR(year_cell.l() + year_lenght);
      years_cells[index] = year_cell;
    }

    auto config = shape_configuration_storage.GetShapeConfiguration("Years Shapes");

    auto node = scene_graph->search_node("year cells");
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    shape->set_shape(years_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupMonthsShapes()
  {
    constexpr size_t number_months = 12;
    size_t store_size = number_months * calendar_config.GetSpanLengthYears();
    std::vector<rectf> months_cells;
    months_cells.resize(store_size);

    for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index) {
      int current_year = calendar_config.GetYear(index);
      boost::gregorian::date first_day_of_year = boost::gregorian::date(current_year, 1, 1);

      for (auto subindex = 0U; subindex < 12; ++subindex) {
        auto current_cell = proportion_frame_layout.GetSubFrame(index, 1);
        rectf month_cell;
        month_cell.setL(
            current_cell.l() +
            static_cast<float>(boost::gregorian::date_period(first_day_of_year,
                                                             first_day_of_year +
                                                                 boost::gregorian::months(subindex))
                                   .length()
                                   .days()) *
                day_width);
        month_cell.setR(
            current_cell.l() +
            static_cast<float>(
                boost::gregorian::date_period(
                    first_day_of_year, first_day_of_year + boost::gregorian::months(subindex + 1))
                    .length()
                    .days()) *
                day_width);
        month_cell.setB(current_cell.b());
        month_cell.setT(current_cell.t());

        size_t store_index = index * 12 + subindex;
        months_cells[store_index] = month_cell;
      }
    }

    auto config = shape_configuration_storage.GetShapeConfiguration("Months Shapes");

    auto node = scene_graph->search_node("month cells");
    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node->get_shape());
    shape->set_shape(months_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupDaysShapes()
  {
    if (calendar_config.IsValidSpan() == false) {
      return;
    }

    int days_index = 0;
    int number_days_cells = calendar_config.GetSpanLengthDays();

    std::vector<rectf> days_cells0(number_days_cells);
    std::vector<rectf> days_cells1(number_days_cells);

    for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index) {
      int current_year = calendar_config.GetYear(index);
      int number_days =
          boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1),
                                        boost::gregorian::date(current_year + 1, 1, 1))
              .length()
              .days();

      for (int subindex = 0; subindex < number_days; ++subindex) {
        float float_subindex = static_cast<float>(subindex);
        auto current_cell = proportion_frame_layout.GetSubFrame(index, 1);

        boost::gregorian::date current_date =
            calendar_config.GetSpanLimitsDate()[0] + boost::gregorian::date_duration(days_index);

        if (current_date.day_of_week() == boost::date_time::Sunday) {
          rectf day_cell;
          day_cell.setL(current_cell.l() + float_subindex * day_width);
          day_cell.setR(day_cell.l() + day_width);
          day_cell.setB(current_cell.b());
          day_cell.setT(current_cell.t());
          days_cells1[days_index] = day_cell;
        } else {
          rectf day_cell;
          day_cell.setL(current_cell.l() + float_subindex * day_width);
          day_cell.setR(day_cell.l() + day_width);
          day_cell.setB(current_cell.b());
          day_cell.setT(current_cell.t());
          days_cells0[days_index] = day_cell;
        }
        ++days_index;
      }
    }

    auto config = shape_configuration_storage.GetShapeConfiguration("Day Shapes");
    auto sunday_config = shape_configuration_storage.GetShapeConfiguration("Sunday Shapes");

    auto node0 = scene_graph->search_node("day cells 0");
    auto shape0 = std::dynamic_pointer_cast<RectanglesShape>(node0->get_shape());
    shape0->set_shape(days_cells0, config.LineWidth());
    shape0->set_color({config.OutlineColor(), config.FillColor()});

    auto node1 = scene_graph->search_node("day cells 1");
    auto shape1 = std::dynamic_pointer_cast<RectanglesShape>(node1->get_shape());
    shape1->set_shape(days_cells1, sunday_config.LineWidth());
    shape1->set_color({sunday_config.OutlineColor(), sunday_config.FillColor()});
  }

  void SetupBarsShape()
  {
    auto node = scene_graph->search_node("bar cells");
    node->remove_children();

    auto number_groups = date_group_store.GetDateGroups().size();
    for (size_t index = 0; index < number_groups; ++index) {
      auto child_node =
          std::make_shared<SceneNode>(std::string("group node ") + std::to_string(index));
      node->add_child(child_node);
    }

    std::vector<std::vector<rectf>> bars_cells(number_groups);

    auto node_labels = scene_graph->search_node("bar labels");
    node_labels->remove_children();

    auto font_shader = graphics_engine->search_shader("Font Shader");
    auto rectangles_shader = graphics_engine->search_shader("Rectangles Shader");

    auto number_bars = data_store.GetNumberBars();
    for (size_t index = 0; index < data_store.GetNumberBars(); ++index) {
      if (calendar_config.IsInSpan(data_store.GetBar(index).GetYear())) {
        auto current_group = data_store.GetBar(index).group;
        auto search_string = std::string("Bar Group ") + std::to_string(current_group);
        auto current_shape_config =
            shape_configuration_storage.GetShapeConfiguration(search_string);

        int row = data_store.GetBar(index).GetYear() - calendar_config.GetSpanLimitsYears()[0];
        auto current_sub_cell = proportion_frame_layout.GetSubFrame(row, 1);

        rectf bar_cell;
        bar_cell.setL(current_sub_cell.l() + data_store.GetBar(index).GetFirstDay() * day_width);
        bar_cell.setR(current_sub_cell.l() + data_store.GetBar(index).GetLastDay() * day_width);
        bar_cell.setB(current_sub_cell.b());
        bar_cell.setT(current_sub_cell.t());

        bars_cells[current_group].push_back(bar_cell);

        std::string label_text = data_store.GetBar(index).GetText();

        auto child_node =
            std::make_shared<SceneNode>(std::string("label node ") + std::to_string(index));
        node_labels->add_child(child_node);

        auto text_shape = std::make_shared<FontShape>(font_shader);
        text_shape->set_font(font);
        child_node->set_shape(text_shape);

        auto current_text_cell = proportion_frame_layout.GetSubFrame(row, 2);
        current_text_cell.setL(bar_cell.l());
        current_text_cell.setR(bar_cell.r());

        text_shape->set_shape_centered(label_text, current_text_cell.getCenter(),
                                       current_text_cell.height());
      }
    }

    auto node_children = node->get_children();
    for (size_t index = 0; index < node_children.size(); ++index) {
      auto shape = std::make_shared<RectanglesShape>(rectangles_shader);
      node_children[index]->set_shape(shape);

      auto search_string = std::string("Bar Group ") + std::to_string(index);
      auto current_shape_config = shape_configuration_storage.GetShapeConfiguration(search_string);

      shape->set_shape(bars_cells[index], current_shape_config.LineWidth());
      shape->set_color({current_shape_config.OutlineColor(), current_shape_config.FillColor()});
    }
  }

  void SetupYearsTotals()
  {
    auto font_shader = graphics_engine->search_shader("Font Shader");
    auto rectangles_shader = graphics_engine->search_shader("Rectangles Shader");

    auto node_cells = scene_graph->search_node("year total cells");

    auto node_text = scene_graph->search_node("year total text");
    node_text->remove_children();

    std::vector<rectf> years_totals_cells;
    years_totals_cells.resize(data_store.GetSpan());

    for (size_t index = 0; index < data_store.GetSpan(); ++index) {
      if (calendar_config.IsInSpan(data_store.GetFirstYear() + index)) {
        int row = data_store.GetFirstYear() + index - calendar_config.GetSpanLimitsYears()[0];
        auto current_cell = proportion_frame_layout.GetSubFrame(row, 0);

        rectf year_total_cell = current_cell;
        year_total_cell.setR(current_cell.l() +
                             static_cast<float>(data_store.GetAnnualTotal(index)) * day_width);
        years_totals_cells[index] = year_total_cell;

        auto current_year = data_store.GetFirstYear() + index;

        int number_days =
            boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1),
                                          boost::gregorian::date(current_year + 1, 1, 1))
                .length()
                .days();

        float percent =
            static_cast<float>(data_store.GetAnnualTotal(index)) / static_cast<float>(number_days);

        std::array<char, 100> year_total_text_buffer;
        auto text_lenght = snprintf(year_total_text_buffer.data(), year_total_text_buffer.size(),
                                    "%.*f %%", 1, percent * 100.0f);
        auto year_total_text = std::string(year_total_text_buffer.data());
        auto year_total_text_width = font->TextWidth(year_total_text, year_total_cell.height());

        rectf year_total_text_cell;
        year_total_text_cell.setL(year_total_cell.r() + current_cell.height());
        year_total_text_cell.setR(year_total_text_cell.l() + year_total_text_width);
        year_total_text_cell.setB(year_total_cell.b());
        year_total_text_cell.setT(year_total_cell.t());

        auto text_shape = std::make_shared<FontShape>(font_shader);
        text_shape->set_font(font);
        text_shape->set_shape_centered(year_total_text, year_total_text_cell.getCenter(),
                                       year_total_text_cell.height());

        auto node_child =
            std::make_shared<SceneNode>(std::string("year total label ") + std::to_string(index));
        node_child->set_shape(text_shape);
        node_text->add_child(node_child);
      }
    }

    auto config = shape_configuration_storage.GetShapeConfiguration("Years Totals");

    auto shape = std::dynamic_pointer_cast<RectanglesShape>(node_cells->get_shape());

    shape->set_shape(years_totals_cells, config.LineWidth());
    shape->set_color({config.OutlineColor(), config.FillColor()});
  }

  void SetupLegend()
  {
    auto font_shader = graphics_engine->search_shader("Font Shader");
    auto rectangles_shader = graphics_engine->search_shader("Rectangles Shader");

    // auto node_area = scene_graph->search_node("legend area").value_or(nullptr);

    auto node_entries = scene_graph->search_node("legend entries");
    node_entries->remove_children();

    auto node_text = scene_graph->search_node("legend text");
    node_text->remove_children();

    size_t number_entrie_frames = (date_group_store.GetDateGroups().size() + 1) * 2;
    std::vector<rectf> legend_entries_frames(number_entrie_frames);
    auto entries_width = legend_frame.width() / static_cast<float>(number_entrie_frames);

    for (size_t index = 0; index < number_entrie_frames; ++index) {
      auto float_index = static_cast<float>(index);
      legend_entries_frames[index] = legend_frame;
      legend_entries_frames[index].setL(legend_frame.l() + entries_width * float_index);
      legend_entries_frames[index].setR(legend_frame.l() + entries_width * float_index +
                                        entries_width);
    }

    std::vector<rectf> bars_cells;
    std::vector<float> bars_cells_shape_linewidths;
    std::vector<glm::vec4> bars_cells_shape_fillcolors;
    std::vector<glm::vec4> bars_cells_shape_outlinecolors;

    auto print_strings = date_group_store.GetDateGroupsNames();
    print_strings.push_back("Annual Sums");

    std::string string_max_lenght = "";
    for (const auto &current_string : print_strings) {
      if (current_string.length() > string_max_lenght.length()) {
        string_max_lenght = current_string;
      }
    }

    auto legend_font_size =
        font->AdjustTextSize(legend_entries_frames[0], string_max_lenght, 0.5f, 0.75f);

    for (size_t index = 0; index < date_group_store.GetDateGroups().size(); ++index) {
      auto node_text_child =
          std::make_shared<SceneNode>(std::string("legend label ") + std::to_string(index));
      node_text->add_child(node_text_child);

      auto shape_text = std::make_shared<FontShape>(font_shader);
      node_text_child->set_shape(shape_text);
      shape_text->set_font(font);
      shape_text->set_shape_centered(date_group_store.GetDateGroups()[index].name,
                                     legend_entries_frames[index * 2].getCenter(),
                                     legend_font_size);

      if (calendar_config.GetSpanLengthYears() > 0) {
        auto current_height = proportion_frame_layout.GetSubFrame(0, 1).height();
        auto current_cell = legend_entries_frames[index * 2 + 1];
        auto current_vertical_center = current_cell.getCenter().y;
        current_cell.setB(current_vertical_center - current_height / 2.f);
        current_cell.setT(current_vertical_center + current_height / 2.f);
        bars_cells.push_back(current_cell);

        auto search_string = std::string("Bar Group ") + std::to_string(index);
        auto current_shape_config =
            shape_configuration_storage.GetShapeConfiguration(search_string);

        auto node_entrie =
            std::make_shared<SceneNode>(std::string("legend bar ") + std::to_string(index));
        node_entries->add_child(node_entrie);

        auto entrie_shape = std::make_shared<RectanglesShape>(rectangles_shader);
        node_entrie->set_shape(entrie_shape);

        entrie_shape->set_shape(current_cell, current_shape_config.LineWidth());
        entrie_shape->set_color(
            {current_shape_config.OutlineColor(), current_shape_config.FillColor()});
      }
    }

    {
      auto node_text_child = std::make_shared<SceneNode>(std::string("legend label year total"));
      node_text->add_child(node_text_child);

      auto shape_text = std::make_shared<FontShape>(font_shader);
      node_text_child->set_shape(shape_text);
      shape_text->set_font(font);
      shape_text->set_shape_centered(
          "Annual sum", legend_entries_frames[legend_entries_frames.size() - 2].getCenter(),
          legend_font_size);

      if (calendar_config.GetSpanLengthYears() > 0) {
        auto current_height = proportion_frame_layout.GetSubFrame(0, 0).height();
        auto current_cell = legend_entries_frames[legend_entries_frames.size() - 1];
        auto current_vertical_center = current_cell.getCenter().y;
        current_cell.setB(current_vertical_center - current_height / 2.f);
        current_cell.setT(current_vertical_center + current_height / 2.f);
        bars_cells.push_back(current_cell);

        auto current_shape_config =
            shape_configuration_storage.GetShapeConfiguration("Years Totals");

        auto node_entrie = std::make_shared<SceneNode>(std::string("legend bar annual sum"));
        node_entries->add_child(node_entrie);

        auto entrie_shape = std::make_shared<RectanglesShape>(rectangles_shader);
        node_entrie->set_shape(entrie_shape);

        entrie_shape->set_shape(current_cell, current_shape_config.LineWidth());
        entrie_shape->set_color(
            {current_shape_config.OutlineColor(), current_shape_config.FillColor()});
      }
    }
  }

private:
  std::shared_ptr<SceneNode> scene_graph;
  std::shared_ptr<Font> font;

  GLCanvas *gl_canvas;
  GraphicsEngine *graphics_engine;

  DateIntervalBundleBarStore data_store;
  DateGroupStore date_group_store;
  CalendarConfigStorage calendar_config;
  ShapeConfigurationStorage shape_configuration_storage;
  TitleConfig title_config;
  ProportionFrameLayout proportion_frame_layout;

  rectf page_size;
  rectf page_margin;
  rectf print_area;
  rectf title_frame;
  rectf page_margin_frame;
  rectf calendar_frame;
  rectf cells_frame;
  rectf x_labels_frame;
  rectf y_labels_frame;
  rectf legend_frame;
  float cell_width;
  float row_height;
  float day_width;
  float labels_font_size;
};
