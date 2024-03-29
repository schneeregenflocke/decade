/*
Decade
Copyright (c) 2019-2022 Marco Peyer

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


#include "gui/opengl_panel.hpp"
#include "graphics/shapes.hpp"
#include "graphics/font.hpp"
#include "frame_layout.hpp"

#include "packages/group_store.hpp"
#include "packages/date_store.hpp"
#include "packages/shape_config.hpp"
#include "packages/calendar_config.hpp"
#include "packages/page_config.hpp"
#include "packages/title_config.hpp"

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



/*class PageShape
{
public:
	explicit PageShape()
	{
		page_shape = std::make_shared<QuadShape>("paper");
	}

private:

	std::shared_ptr<QuadShape> page_shape;
};*/


class CalendarPage
{
public:

	CalendarPage(GLCanvas* gl_canvas, const std::string& font_filepath) :
		gl_canvas(gl_canvas)
	{
		graphics_engine = gl_canvas->GraphicsEnginePtr();

		page_shape = std::make_shared<QuadShape>("paper");

		font_loader = std::make_shared<Font>(font_filepath);
		title_font_shape = std::make_shared<FontShape>("title text", font_loader);
		
		print_area_shape = std::make_shared<RectanglesShape>("print area");
		title_area_shape = std::make_shared<RectanglesShape>("title area");
		days_cells_shape = std::make_shared<RectanglesShape>("day cells");
		months_cells_shape = std::make_shared<RectanglesShape>("month cells");
		years_cells_shape = std::make_shared<RectanglesShape>("year cells");
		bars_cells_shape = std::make_shared<RectanglesShape>("bar cells");
		years_totals_shape = std::make_shared<RectanglesShape>("year total cells");
		
		row_labels_shape = std::make_shared<RectanglesShape>("year label cells");
		column_labels_shape = std::make_shared<RectanglesShape>("month label cells");
		legend_shape = std::make_shared<RectanglesShape>("legend frame");
		legend_entries_shape = std::make_shared<RectanglesShape>("legend label frame");

		graphics_engine->AddShape(page_shape);
		graphics_engine->AddShape(title_font_shape);
		graphics_engine->AddShape(print_area_shape);
		graphics_engine->AddShape(title_area_shape);
		graphics_engine->AddShape(days_cells_shape);
		graphics_engine->AddShape(months_cells_shape);
		graphics_engine->AddShape(years_cells_shape);
		graphics_engine->AddShape(bars_cells_shape);
		graphics_engine->AddShape(years_totals_shape);
		graphics_engine->AddShape(row_labels_shape);
		graphics_engine->AddShape(column_labels_shape);
		graphics_engine->AddShape(legend_shape);
		graphics_engine->AddShape(legend_entries_shape);
	}

	void ReceiveDateGroups(const std::vector<DateGroup>& date_groups)
	{
		date_group_store.ReceiveDateGroups(date_groups);
		data_store.ReceiveDateGroups(date_groups);
		Update();
	}

	void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
	{
		data_store.ReceiveDateIntervalBundles(date_interval_bundles);
		Update();
	}

	void ReceivePageSetup(const PageSetupConfig& page_setup_config)
	{
		this->page_size = rectf::from_dimension(page_setup_config.size[0], page_setup_config.size[1]);
		this->page_margin = rectf(page_setup_config.margins[0], page_setup_config.margins[1], page_setup_config.margins[2], page_setup_config.margins[3]);
		Update();
	}

	void ReceiveFont(const std::string& font_filepath)
	{
		font_loader.reset(new Font(font_filepath));
		Update();
	}

	void ReceiveTitleConfig(const TitleConfig& title_config)
	{
		this->title_config = title_config;
		Update();
	}

	void ReceiveCalendarConfig(const CalendarConfigStorage& calendar_config)
	{
		this->calendar_config = calendar_config;
		Update();
	}

	void ReceiveShapeConfigurationStorage(const ShapeConfigurationStorage& shape_configuration_storage)
	{
		this->shape_configuration_storage = shape_configuration_storage;
		Update();
	}

	void Update()
	{
		glm::vec4 page_shape_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		page_shape->SetShape(page_size, page_shape_color);

		print_area = page_size.reduce(page_margin);

		title_frame = print_area;
		title_frame.setB(title_frame.t() - title_config.frame_height);

		page_margin_frame = print_area;
		page_margin_frame.setT(title_frame.b());
		//const rectf page_margin_frame_margin(0.0f, 0.0f, 0.0f, 0.f);
		//page_margin_frame = page_margin_frame.reduce(page_margin_frame_margin);

		calendar_frame = page_margin_frame;
		const rectf calendar_frame_margin = rectf(0.0f, 5.0f, 0.0f, 0.0f);
		calendar_frame = calendar_frame.reduce(calendar_frame_margin);

		if (calendar_config.auto_calendar_span == true && data_store.is_empty() == false)
		{
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

		x_labels_frame = calendar_frame.reduce(rectf(cell_width, 0.f, row_height, cells_frame.height()));
		y_labels_frame = calendar_frame.reduce(rectf(0.f, cells_frame.width(), row_height * 2.f, 0.f));
		legend_frame = calendar_frame.reduce(rectf(cell_width, 0.f, 0.f, cells_frame.height() + row_height));

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
		print_area_shape->SetShapes(page_margin_frame, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupTitleShape()
	{
		auto config = shape_configuration_storage.GetShapeConfiguration("Title Frame");
		title_area_shape->SetShapes(title_frame, config.LineWidth(), config.FillColor(), config.OutlineColor());

		
		graphics_engine->RemoveShape(title_font_shape);
		
		title_font_shape = std::make_shared<FontShape>("title text", font_loader);
		graphics_engine->AddShape(title_font_shape);

		title_font_shape->SetShapeCentered(title_config.title_text, title_frame.getCenter(), title_frame.height() * title_config.font_size_ratio);
	}

	void SetupCalendarLabelsShape()
	{
		std::array<char, 100> buf;
		std::string format = "%b";
		std::array<std::string, 12> months_names;
		for (size_t index = 0; index < months_names.size(); ++index)
		{
			std::tm month_tm = { 0 };
			month_tm.tm_mon = index;
			auto len = std::strftime(buf.data(), sizeof(buf), format.c_str(), &month_tm);
			months_names[index] = buf.data();
		}

		graphics_engine->RemoveShapes(month_label_text);
		month_label_text.clear();
		const size_t number_months = 12;
		for (size_t index = 0; index < number_months; ++index)
		{
			month_label_text.push_back(std::make_shared<FontShape>(std::string("month label ") + std::to_string(index), font_loader));
			graphics_engine->AddShape(month_label_text[index]);
		}

		std::vector<rectf> x_label_frames(12);
		labels_font_size = font_loader->AdjustTextSize(rectf::from_dimension(cell_width, row_height), "00000", 0.5f, 0.75f);
		for (size_t index = 0; index < months_names.size(); ++index)
		{
			float float_index = static_cast<float>(index);
			x_label_frames[index].setL(x_labels_frame.l() + cell_width * float_index);
			x_label_frames[index].setR(x_labels_frame.l() + cell_width * float_index + cell_width);
			x_label_frames[index].setB(x_labels_frame.b());
			x_label_frames[index].setT(x_labels_frame.t());
			month_label_text[index]->SetShapeCentered(months_names[index], x_label_frames[index].getCenter(), labels_font_size);
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Calendar Labels");
		column_labels_shape->SetShapes(x_label_frames, config.LineWidth(), config.FillColor(), config.OutlineColor());


		graphics_engine->RemoveShapes(annual_labels_text);
		annual_labels_text.clear();
		for (size_t index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			annual_labels_text.push_back(std::make_shared<FontShape>(std::string("annual label ") + std::to_string(index), font_loader));
			graphics_engine->AddShape(annual_labels_text[index]);
		}
		
		std::vector<rectf> y_labels_rows(calendar_config.GetSpanLengthYears());
		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			float float_index = static_cast<float>(index);
			y_labels_rows[index] = rectf(
				y_labels_frame.l(),
				y_labels_frame.r(),
				y_labels_frame.b() + row_height * float_index,
				y_labels_frame.b() + row_height * float_index + row_height
			);

			std::string current_year_text = std::to_string(calendar_config.GetYear(index));
			annual_labels_text[index]->SetShapeCentered(current_year_text, y_labels_rows[index].getCenter(), labels_font_size);
		}
		row_labels_shape->SetShapes(y_labels_rows, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupYearsShapes()
	{
		std::vector<rectf> years_cells;
		years_cells.resize(calendar_config.GetSpanLengthYears());

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			int current_year = calendar_config.GetYear(index);
			int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();
			float year_lenght = static_cast<float>(number_days) * day_width;
			proportion_frame_layout.GetSubFrame(index, 1);
			rectf year_cell = proportion_frame_layout.GetSubFrame(index, 1);
			year_cell.setR(year_cell.l() + year_lenght);
			years_cells[index] = year_cell;
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Years Shapes");
		years_cells_shape->SetShapes(years_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupMonthsShapes()
	{
		const size_t number_months = 12;
		size_t store_size = number_months * calendar_config.GetSpanLengthYears();
		std::vector<rectf> months_cells;
		months_cells.resize(store_size);

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			int current_year = calendar_config.GetYear(index);
			boost::gregorian::date first_day_of_year = boost::gregorian::date(current_year, 1, 1);

			for (auto subindex = 0U; subindex < 12; ++subindex)
			{
				auto current_cell = proportion_frame_layout.GetSubFrame(index, 1);
				rectf month_cell;
				month_cell.setL(current_cell.l() + static_cast<float>(boost::gregorian::date_period(first_day_of_year, first_day_of_year + boost::gregorian::months(subindex)).length().days()) * day_width);
				month_cell.setR(current_cell.l() + static_cast<float>(boost::gregorian::date_period(first_day_of_year, first_day_of_year + boost::gregorian::months(subindex + 1)).length().days()) * day_width);
				month_cell.setB(current_cell.b());
				month_cell.setT(current_cell.t());

				size_t store_index = index * 12 + subindex;
				months_cells[store_index] = month_cell;
			}
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Months Shapes");
		months_cells_shape->SetShapes(months_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupDaysShapes()
	{
		if (calendar_config.IsValidSpan() == false)
		{
			return;
		}

		int days_index = 0;
		int number_days_cells = calendar_config.GetSpanLengthDays();
		std::vector<rectf> days_cells(number_days_cells);
		std::vector<float> days_cells_shape_linewidths(number_days_cells);
		std::vector<glm::vec4> days_cells_shape_fillcolors(number_days_cells);
		std::vector<glm::vec4> days_cells_shape_outlinecolors(number_days_cells);

		auto config = shape_configuration_storage.GetShapeConfiguration("Day Shapes");
		auto sunday_config = shape_configuration_storage.GetShapeConfiguration("Sunday Shapes");

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			int current_year = calendar_config.GetYear(index);
			int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();

			for (int subindex = 0; subindex < number_days; ++subindex)
			{
				float float_subindex = static_cast<float>(subindex);
				auto current_cell = proportion_frame_layout.GetSubFrame(index, 1);

				rectf day_cell;
				day_cell.setL(current_cell.l() + float_subindex * day_width);
				day_cell.setR(day_cell.l() + day_width);
				day_cell.setB(current_cell.b());
				day_cell.setT(current_cell.t());
				days_cells[days_index] = day_cell;

				boost::gregorian::date current_date = calendar_config.GetSpanLimitsDate()[0] + boost::gregorian::date_duration(days_index);
				if (current_date.day_of_week() == boost::date_time::Sunday)
				{
					days_cells_shape_linewidths[days_index] = sunday_config.LineWidth();
					days_cells_shape_fillcolors[days_index] = sunday_config.FillColor();
					days_cells_shape_outlinecolors[days_index] = sunday_config.OutlineColor();
				}
				else
				{
					days_cells_shape_linewidths[days_index] = config.LineWidth();
					days_cells_shape_fillcolors[days_index] = config.FillColor();
					days_cells_shape_outlinecolors[days_index] = config.OutlineColor();
				}
				++days_index;
			}
		}
		days_cells_shape->SetShapes(days_cells, days_cells_shape_linewidths, days_cells_shape_fillcolors, days_cells_shape_outlinecolors);
	}

	void SetupBarsShape()
	{
		auto number_bars = data_store.GetNumberBars();

		std::vector<rectf> bars_cells;
		std::vector<float> bars_cells_shape_linewidths;
		std::vector<glm::vec4> bars_cells_shape_fillcolors;
		std::vector<glm::vec4> bars_cells_shape_outlinecolors;

		graphics_engine->RemoveShapes(bar_labels_text);
		bar_labels_text.clear();

		for (size_t index = 0; index < data_store.GetNumberBars(); ++index)
		{
			if (calendar_config.IsInSpan(data_store.GetBar(index).GetYear()))
			{
				auto current_group = data_store.GetBar(index).group;
				auto search_string = std::string("Bar Group ") + std::to_string(current_group);
				auto current_shape_config = shape_configuration_storage.GetShapeConfiguration(search_string);

				bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
				bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
				bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());

				int row = data_store.GetBar(index).GetYear() - calendar_config.GetSpanLimitsYears()[0];
				auto current_sub_cell = proportion_frame_layout.GetSubFrame(row, 1);

				rectf bar_cell;
				bar_cell.setL(current_sub_cell.l() + data_store.GetBar(index).GetFirstDay() * day_width);
				bar_cell.setR(current_sub_cell.l() + data_store.GetBar(index).GetLastDay() * day_width);
				bar_cell.setB(current_sub_cell.b());
				bar_cell.setT(current_sub_cell.t());

				bars_cells.push_back(bar_cell);
				std::string numsText = data_store.GetBar(index).GetText();
				bar_labels_text.push_back(std::make_shared<FontShape>(std::string("bar label text ") + std::to_string(index), font_loader));
				graphics_engine->AddShape(bar_labels_text[index]);

				auto current_text_cell = proportion_frame_layout.GetSubFrame(row, 2);
				current_text_cell.setL(bar_cell.l());
				current_text_cell.setR(bar_cell.r());

				bar_labels_text.back()->SetShapeCentered(numsText, current_text_cell.getCenter(), current_text_cell.height());
			}
		}
		bars_cells_shape->SetShapes(bars_cells, bars_cells_shape_linewidths, bars_cells_shape_fillcolors, bars_cells_shape_outlinecolors);
	}

	void SetupYearsTotals()
	{
		graphics_engine->RemoveShapes(years_totals_text);
		years_totals_text.clear();

		std::vector<rectf> years_totals_cells;
		years_totals_cells.resize(data_store.GetSpan());

		for (size_t index = 0; index < data_store.GetSpan(); ++index)
		{
			if (calendar_config.IsInSpan(data_store.GetFirstYear() + index))
			{
				int row = data_store.GetFirstYear() + index - calendar_config.GetSpanLimitsYears()[0];
				auto current_cell = proportion_frame_layout.GetSubFrame(row, 0);

				rectf year_total_cell = current_cell;
				year_total_cell.setR(current_cell.l() + static_cast<float>(data_store.GetAnnualTotal(index)) * day_width);
				years_totals_cells[index] = year_total_cell;

				auto current_year = data_store.GetFirstYear() + index;
				int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();

				float percent = static_cast<float>(data_store.GetAnnualTotal(index)) / static_cast<float>(number_days);

				std::array<char, 100> year_total_text_buffer;
				auto text_lenght = snprintf(year_total_text_buffer.data(), year_total_text_buffer.size(), "%.*f %%", 1, percent * 100.0f);
				auto year_total_text = std::string(year_total_text_buffer.data());
				auto year_total_text_width = font_loader->TextWidth(year_total_text, year_total_cell.height());

				rectf year_total_text_cell;
				year_total_text_cell.setL(year_total_cell.r() + current_cell.height());
				year_total_text_cell.setR(year_total_text_cell.l() + year_total_text_width);
				year_total_text_cell.setB(year_total_cell.b());
				year_total_text_cell.setT(year_total_cell.t());

				years_totals_text.push_back(std::make_shared<FontShape>(std::string("year total label ") + std::to_string(index), font_loader));
				graphics_engine->AddShape(years_totals_text[index]);
				years_totals_text.back()->SetShapeCentered(year_total_text, year_total_text_cell.getCenter(), year_total_text_cell.height());
			}
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Years Totals");
		years_totals_shape->SetShapes(years_totals_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupLegend()
	{
		size_t number_entrie_frames = (date_group_store.GetDateGroups().size() + 1/*annual_total*/) * 2;
		std::vector<rectf> legend_entries_frames(number_entrie_frames);
		auto entries_width = legend_frame.width() / static_cast<float>(number_entrie_frames);

		for (size_t index = 0; index < number_entrie_frames; ++index)
		{
			auto float_index = static_cast<float>(index);
			legend_entries_frames[index] = legend_frame;
			legend_entries_frames[index].setL(legend_frame.l() + entries_width * float_index);
			legend_entries_frames[index].setR(legend_frame.l() + entries_width * float_index + entries_width);
		}

		std::vector<rectf> bars_cells;
		std::vector<float> bars_cells_shape_linewidths;
		std::vector<glm::vec4> bars_cells_shape_fillcolors;
		std::vector<glm::vec4> bars_cells_shape_outlinecolors;

		auto print_strings = date_group_store.GetDateGroupsNames();
		print_strings.push_back("Annual Sums");

		std::string string_max_lenght = "";
		for (const auto& current_string : print_strings)
		{
			if (current_string.length() > string_max_lenght.length())
			{
				string_max_lenght = current_string;
			}
		}

		auto legend_font_size = font_loader->AdjustTextSize(legend_entries_frames[0], string_max_lenght, 0.5f, 0.75f);

		graphics_engine->RemoveShapes(legend_text);
		legend_text.clear();

		for (size_t index = 0; index < date_group_store.GetDateGroups().size(); ++index)
		{
			legend_text.push_back(std::make_shared<FontShape>(std::string("legend label ") + std::to_string(index), font_loader));
			graphics_engine->AddShape(legend_text.back());
			legend_text.back()->SetShapeCentered(date_group_store.GetDateGroups()[index].name, legend_entries_frames[index * 2].getCenter(), legend_font_size);

			if (calendar_config.GetSpanLengthYears() > 0)
			{
				auto current_height = proportion_frame_layout.GetSubFrame(0, 1).height();
				auto current_cell = legend_entries_frames[index * 2 + 1];
				auto current_vertical_center = current_cell.getCenter().y;
				current_cell.setB(current_vertical_center - current_height / 2.f);
				current_cell.setT(current_vertical_center + current_height / 2.f);
				bars_cells.push_back(current_cell);

				auto search_string = std::string("Bar Group ") + std::to_string(index);
				auto current_shape_config = shape_configuration_storage.GetShapeConfiguration(search_string);

				bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
				bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
				bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());
			}
			
			legend_text.push_back(std::make_shared<FontShape>(std::string("annual sums ") + std::to_string(index), font_loader));
			graphics_engine->AddShape(legend_text.back());
			legend_text.back()->SetShapeCentered("Annual Sums", legend_entries_frames[legend_entries_frames.size() - 2].getCenter(), legend_font_size);

			if (calendar_config.GetSpanLengthYears() > 0)
			{
				auto current_height = proportion_frame_layout.GetSubFrame(0, 0).height();
				auto current_cell = legend_entries_frames[legend_entries_frames.size() - 1];
				auto current_vertical_center = current_cell.getCenter().y;
				current_cell.setB(current_vertical_center - current_height / 2.f);
				current_cell.setT(current_vertical_center + current_height / 2.f);
				bars_cells.push_back(current_cell);

				auto current_shape_config = shape_configuration_storage.GetShapeConfiguration("Years Totals");

				bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
				bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
				bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());
			}

		}
		legend_entries_shape->SetShapes(bars_cells, bars_cells_shape_linewidths, bars_cells_shape_fillcolors, bars_cells_shape_outlinecolors);
	}

private:

	GLCanvas* gl_canvas;
	GraphicsEngine* graphics_engine;

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

	////////////////////////////////////////////////////////////////////////////////

	
	std::shared_ptr<Font> font_loader;

	std::shared_ptr<QuadShape> page_shape;

	std::shared_ptr<FontShape> title_font_shape;
	std::vector<std::shared_ptr<FontShape>> years_totals_text;
	std::vector<std::shared_ptr<FontShape>> month_label_text;
	std::vector<std::shared_ptr<FontShape>> annual_labels_text;
	std::vector<std::shared_ptr<FontShape>> bar_labels_text;
	std::vector<std::shared_ptr<FontShape>> legend_text;

	std::shared_ptr<RectanglesShape> print_area_shape;
	std::shared_ptr<RectanglesShape> title_area_shape;
	std::shared_ptr<RectanglesShape> years_cells_shape;
	std::shared_ptr<RectanglesShape> months_cells_shape;
	std::shared_ptr<RectanglesShape> days_cells_shape;
	std::shared_ptr<RectanglesShape> bars_cells_shape;
	std::shared_ptr<RectanglesShape> years_totals_shape;
	std::shared_ptr<RectanglesShape> row_labels_shape;
	std::shared_ptr<RectanglesShape> column_labels_shape;
	std::shared_ptr<RectanglesShape> legend_shape;
	std::shared_ptr<RectanglesShape> legend_entries_shape;
	std::shared_ptr<RectanglesShape> sub_frames_shape;
};

