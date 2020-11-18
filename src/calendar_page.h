/*
Decade
Copyright (c) 2019-2020 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/gpl-3.0.txt>.
*/

#pragma once

#include "graphics/graphic_engine.h"
#include "graphics/shapes.h"
#include "graphics/font.h"

#include "date_group_store.h"
#include "dates_store.h"
#include "shape_config.h"
#include "calendar_config.h"
#include "page_config.h"

#include <string>
#include <memory>
#include <ctime>
#include <algorithm>
#include <array>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <numeric>


class RowFrames
{
public:

	void SetupRowFrames(const rect4& main_frame, const size_t number_row_frames);
	void SetupSubFrames(const std::vector<float>& proportions);

	rect4 GetSubFrame(const size_t row, const size_t sub) const;

private:

	std::vector<float> Section(const std::vector<float>& proportions, float value) const;

	std::vector<rect4> row_frames;
	std::vector<rect4> sub_frames;

	size_t number_sub_frames;
};



class CalendarPage
{
public:

	CalendarPage(GraphicEngine* graphic_engine);

	void ReceiveDateGroups(const std::vector<DateGroup>& date_groups);
	void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles);
	void ReceivePageSetup(const PageSetupConfig& page_setup_config);
	void ReceiveFont(const std::string& font_path);
	void ReceiveTitleFrameHeight(float height);
	void ReceiveTitleFontSizeRatio(float ratio);
	void ReceiveTitleText(const std::string& text);
	void ReceiveTitleTextColor(const std::array<float, 4>& title_text_color);
	void ReceiveCalendarConfig(const CalendarConfig& calendar_config);

	void ReceiveRectangleShapeConfig(const std::vector<RectangleShapeConfig>& configs);
	RectangleShapeConfig GetShapeConfig(const std::string& name);

	void Update();

	void SetupPrintAreaShape();
	void SetupTitleShape();
	void SetupCalendarLabelsShape();
	void SetupYearsShapes();
	void SetupMonthsShapes();
	void SetupDaysShapes();
	void SetupBarsShape();
	void SetupYearsTotals();
	void SetupLegend();

private:

	GraphicEngine* graphic_engine;

	DateIntervalBundleBarStore data_store;
	CalendarConfig calendar_config;
	
	//CalendarSpan calendar_span;
	RowFrames row_frames;
	
	rect4 page_size;
	rect4 page_margin;
	rect4 print_area;
	float title_frame_height;
	rect4 title_frame;
	rect4 page_margin_frame;
	rect4 calendar_frame;
	rect4 cells_frame;
	rect4 x_labels_frame;
	rect4 y_labels_frame;
	rect4 legend_frame;
	float cell_width;
	float row_height;
	float day_width;
	float title_font_size_ratio;
	std::string title_text;
	glm::vec4 title_text_color;
	float labels_font_size;

	////////////////////////////////////////////////////////////////////////////////

	
	std::vector<RectangleShapeConfig> element_configurations;
	std::vector<RectangleShapeConfig> bar_shape_configs;

	DateGroupStore date_group_store;
	
	////////////////////////////////////////////////////////////////////////////////

	std::shared_ptr<FontLoader> font_loader;
	std::shared_ptr<FontShape> title_font_shape;

	std::shared_ptr<QuadShape> page_shape;

	std::shared_ptr<RectanglesShape> print_area_shape;
	std::shared_ptr<RectanglesShape> title_area_shape;
	std::shared_ptr<RectanglesShape> years_cells_shape;
	std::shared_ptr<RectanglesShape> months_cells_shape;
	std::shared_ptr<RectanglesShape> days_cells_shape;
	std::shared_ptr<RectanglesShape> bars_cells_shape;

	std::shared_ptr<RectanglesShape> years_totals_shape;
	std::vector< std::shared_ptr<FontShape> > years_totals_text;

	std::vector< std::shared_ptr<FontShape> > month_label_text;
	std::vector< std::shared_ptr<FontShape> > annual_labels_text;
	std::vector< std::shared_ptr<FontShape> > bar_labels_text;

	std::vector< std::shared_ptr<FontShape> > legend_text;

	std::shared_ptr<RectanglesShape> row_labels_shape;
	std::shared_ptr<RectanglesShape> column_labels_shape;

	std::shared_ptr<RectanglesShape> legend_shape;
	std::shared_ptr<RectanglesShape> legend_entries_shape;

	std::shared_ptr<RectanglesShape> sub_frames_shape;
};

