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


#ifndef CALENDAR_H
#define CALENDAR_H

#include "graphics/graphic_engine.h"
#include "graphics/shapes.h"
#include "graphics/font.h"

#include "date_group_store.h"
#include "dates_store.h"
#include "shape_config.h"
#include "calendar_config.h"

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

////////////////////////////////////////



class CalendarSpan
{
public:

	CalendarSpan() :
		lower_limit(0),
		upper_limit(0),
		valid_span(false)
	{}

	void SetSpan(const int lower_limit, const int upper_limit);
	int GetSpanSize() const;
	
	bool IsValidSpan() const;
	bool IsInSpan(const int year) const;
	
	int GetFirstYear() const;
	int GetLastYear() const;
	
private:
	int lower_limit;
	int upper_limit;
	bool valid_span;
};





class RowFrames
{
public:

	void SetupRowFrames(const rect4& main_frame, size_t num_frames);
	void SetupSubFrames(const std::vector<float>& proportions);

	rect4 GetSubFrame(size_t row, size_t sub);
	std::vector<rect4> GetSubFrames();

	std::vector<float> Section(const std::vector<float>& weights, float value);

private:

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

	void SlotPageSize(const std::array<float, 2>& page_size);
	void SlotPageMargins(const std::array<float, 4>& page_margins);

	void SlotSelectFont(const std::string& font_path);

	void SlotTitleFrameHeight(float height);
	void SlotTitleFontSizeRatio(float ratio);
	void SlotTitleText(const std::wstring& text);
	void SlotTitleTextColor(const std::array<float, 4>& title_text_color);

	void SlotRectangleShapeConfig(const std::vector<RectangleShapeConfig>& configs);

	void SlotCalendarConfig(const CalendarConfig& config);

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

	DateIntervalBundleBarStore data_store;

	GraphicEngine* graphic_engine;
	CalendarSpan calendar_span;

	

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

	CalendarConfig calendar_config;
	
	float cell_width;
	float row_height;
	float day_width;

	float title_font_size_ratio;

	std::wstring title_text;
	glm::vec4 title_text_color;

	float labels_font_size;

	RowFrames row_frames;

	////////////////////////////////////////////////////////////////////////////////

	RectangleShapeConfig GetShapeConfig(const std::wstring& name);
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


#endif /* CALENDAR_H */
