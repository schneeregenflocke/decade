/*
Decade
Copyright (c) 2019-2021 Marco Peyer

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.
*/

#pragma once

#include "graphics/graphic_engine.h"
#include "graphics/shapes.h"
#include "graphics/font.h"

#include "packages/group_store.h"
#include "packages/date_store.h"
#include "packages/shape_config.h"
#include "packages/calendar_config.h"
#include "packages/page_config.h"
#include "packages/title_config.h"

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

	void SetupRowFrames(const rect4& main_frame, const size_t number_row_frames)
	{
		row_frames.resize(number_row_frames);

		auto row_height = main_frame.Height() / static_cast<float>(number_row_frames);

		for (size_t index = 0; index < row_frames.size(); ++index)
		{
			auto float_index = static_cast<float>(index);

			row_frames[index].Left(main_frame.Left());
			row_frames[index].Right(main_frame.Right());

			auto current_bottom = main_frame.Bottom() + float_index * row_height;
			row_frames[index].Bottom(current_bottom);
			row_frames[index].Top(current_bottom + row_height);
		}
	}
	void SetupSubFrames(const std::vector<float>& proportions)
	{
		number_sub_frames = (proportions.size() - 1) / 2;

		sub_frames.resize(row_frames.size() * number_sub_frames);

		for (size_t index = 0; index < row_frames.size(); ++index)
		{
			std::vector<float> sections = Section(proportions, row_frames[index].Height());

			std::vector<float> cumulative_sections(sections.size());

			for (size_t subindex = 0; subindex < cumulative_sections.size(); ++subindex)
			{
				cumulative_sections[subindex] = std::accumulate(sections.cbegin(), sections.cbegin() + subindex, 0.f);
			}

			for (size_t subindex = 0; subindex < number_sub_frames; ++subindex)
			{
				sub_frames[index * number_sub_frames + subindex].Left(row_frames[index].Left());
				sub_frames[index * number_sub_frames + subindex].Right(row_frames[index].Right());

				sub_frames[index * number_sub_frames + subindex].Bottom(row_frames[index].Bottom() + cumulative_sections[subindex * 2 + 1]);
				sub_frames[index * number_sub_frames + subindex].Top(row_frames[index].Bottom() + cumulative_sections[subindex * 2 + 2]);
			}
		}
	}

	rect4 GetSubFrame(const size_t row, const size_t sub) const
	{
		return sub_frames[number_sub_frames * row + sub];
	}

private:

	std::vector<float> Section(const std::vector<float>& proportions, float value) const
	{
		auto sum = std::accumulate(proportions.cbegin(), proportions.cend(), 0.f);

		std::vector<float> sections;
		sections.reserve(proportions.size());

		for (const auto& proportion : proportions)
		{
			sections.push_back((proportion / sum) * value);
		}

		return sections;
	}

	std::vector<rect4> row_frames;
	std::vector<rect4> sub_frames;

	size_t number_sub_frames;
};



class CalendarPage
{
public:

	CalendarPage(GraphicEngine* graphic_engine) :
		graphic_engine(graphic_engine)
	{
		page_shape = graphic_engine->AddShape<QuadShape>();

		print_area_shape = graphic_engine->AddShape<RectanglesShape>();
		title_area_shape = graphic_engine->AddShape<RectanglesShape>();

		days_cells_shape = graphic_engine->AddShape<RectanglesShape>();
		months_cells_shape = graphic_engine->AddShape<RectanglesShape>();
		years_cells_shape = graphic_engine->AddShape<RectanglesShape>();

		bars_cells_shape = graphic_engine->AddShape<RectanglesShape>();
		years_totals_shape = graphic_engine->AddShape<RectanglesShape>();

		title_font_shape = graphic_engine->AddShape<FontShape>();

		month_label_text = graphic_engine->AddShapes<FontShape>(12);

		row_labels_shape = graphic_engine->AddShape<RectanglesShape>();
		column_labels_shape = graphic_engine->AddShape<RectanglesShape>();

		legend_shape = graphic_engine->AddShape<RectanglesShape>();
		legend_entries_shape = graphic_engine->AddShape<RectanglesShape>();

		sub_frames_shape = graphic_engine->AddShape<RectanglesShape>();
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
		this->page_size = rect4(page_setup_config.size[0], page_setup_config.size[1]);
		this->page_margin = rect4(page_setup_config.margins[0], page_setup_config.margins[1], page_setup_config.margins[2], page_setup_config.margins[3]);
		Update();
	}

	void ReceiveFont(const std::string& font_path)
	{
		font_loader.reset(std::make_unique<FontLoader>(font_path).release());

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
		const float view_size_scale = 1.1f;
		rect4 view_size = page_size.Scale(view_size_scale);
		graphic_engine->GetViewRef().SetMinRect(view_size);

		glm::vec4 page_shape_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		page_shape->SetShape(page_size, page_shape_color);

		print_area = page_size.Reduce(page_margin);

		title_frame = print_area;
		title_frame.Bottom(title_frame.Top() - title_config.frame_height);

		page_margin_frame = print_area;
		page_margin_frame.Top(title_frame.Bottom());
		//rect4 page_margin_frame_margin = rect4(0.0f, 0.0f, 0.0f, 5.f);
		//page_margin_frame = page_margin_frame.Reduce(page_margin_frame_margin);

		calendar_frame = page_margin_frame;
		rect4 calendar_frame_margin = rect4(0.0f, 0.0f, 3.0f, 2.0f);
		calendar_frame = calendar_frame.Reduce(calendar_frame_margin);


		if (calendar_config.auto_calendar_span == true && data_store.is_empty() == false)
		{
			calendar_config.SetSpan(data_store.GetFirstYear(), data_store.GetLastYear());
		}


		const size_t additional_rows = 2;
		size_t number_rows = additional_rows + calendar_config.GetSpanLengthYears();

		cell_width = calendar_frame.Width() / 13.0f;
		row_height = calendar_frame.Height() / static_cast<float>(number_rows);

		cells_frame = calendar_frame.Reduce(rect4(cell_width, row_height * 2.0f, 0.0f, 0.0f));

		row_frames.SetupRowFrames(cells_frame, calendar_config.GetSpanLengthYears());
		row_frames.SetupSubFrames(calendar_config.spacing_proportions);

		day_width = cells_frame.Width() / 366.f;
		//day_width = row_frames.GetSubFrame(3, 0).Height();

		x_labels_frame = calendar_frame.Reduce(rect4(cell_width, row_height, 0.f, cells_frame.Height()));
		y_labels_frame = calendar_frame.Reduce(rect4(0.f, row_height * 2.f, cells_frame.Width(), 0.f));

		legend_frame = calendar_frame.Reduce(rect4(cell_width, 0.f, 0.f, cells_frame.Height() + row_height));

		///////////////////////////////////////

		//sub_frames_shape->SetShapes(row_frames.GetSubFrames(), 0.1f, vec4(1.f, 1.f, 1.f, 0.f), vec4(0.25f, 0.25f, 0.75f, 1.f));

		SetupPrintAreaShape();
		SetupTitleShape();
		SetupCalendarLabelsShape();
		SetupDaysShapes();
		SetupMonthsShapes();
		SetupYearsShapes();
		SetupBarsShape();
		SetupYearsTotals();
		SetupLegend();

		graphic_engine->Refresh();
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

		title_font_shape->SetFont(font_loader);
		title_font_shape->SetShapeCentered(title_config.title_text, title_frame.Center(), title_frame.Height() * title_config.font_size_ratio);
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

		std::vector<rect4> x_label_frames(12);

		if (font_loader)
		{
			labels_font_size = font_loader->AdjustTextSize(rect4(cell_width, row_height), "00000", 0.5f, 0.75f);
		}

		for (size_t index = 0; index < months_names.size(); ++index)
		{
			float float_index = static_cast<float>(index);

			x_label_frames[index].Left(x_labels_frame.Left() + cell_width * float_index);
			x_label_frames[index].Bottom(x_labels_frame.Bottom());
			x_label_frames[index].Right(x_labels_frame.Left() + cell_width * float_index + cell_width);
			x_label_frames[index].Top(x_labels_frame.Top());

			month_label_text[index]->SetFont(font_loader);
			month_label_text[index]->SetShapeCentered(months_names[index], x_label_frames[index].Center(), labels_font_size);
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Calendar Labels");

		column_labels_shape->SetShapes(x_label_frames, config.LineWidth(), config.FillColor(), config.OutlineColor());

		graphic_engine->RemoveShapes(annual_labels_text);
		annual_labels_text = graphic_engine->AddShapes<FontShape>(calendar_config.GetSpanLengthYears());

		std::vector<rect4> y_labels_rows(calendar_config.GetSpanLengthYears());
		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			float float_index = static_cast<float>(index);

			y_labels_rows[index] = rect4(
				y_labels_frame.Left(),
				y_labels_frame.Bottom() + row_height * float_index,
				y_labels_frame.Right(),
				y_labels_frame.Bottom() + row_height * float_index + row_height
			);

			std::string current_year_text = std::to_string(calendar_config.GetYear(index));

			annual_labels_text[index]->SetFont(font_loader);
			annual_labels_text[index]->SetShapeCentered(current_year_text, y_labels_rows[index].Center(), labels_font_size);
		}

		row_labels_shape->SetShapes(y_labels_rows, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupYearsShapes()
	{
		std::vector<rect4> years_cells;
		years_cells.resize(calendar_config.GetSpanLengthYears());

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			//float indexfloat = static_cast<float>(index);
			int current_year = calendar_config.GetYear(index);
			int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();
			float year_lenght = static_cast<float>(number_days) * day_width;

			row_frames.GetSubFrame(index, 1);

			rect4 year_cell = row_frames.GetSubFrame(index, 1);
			//year_cell.Left(cells_frame.Left());
			//year_cell.Bottom(cells_frame.Bottom() + row_height * float_index + row_height / 4.f);
			year_cell.Right(year_cell.Left() + year_lenght);
			//year_cell.Top(year_cell.Bottom() + row_height / 4.f);

			years_cells[index] = year_cell;
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Years Shapes");

		years_cells_shape->SetShapes(years_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupMonthsShapes()
	{
		const size_t number_months = 12;
		size_t store_size = number_months * calendar_config.GetSpanLengthYears();
		std::vector<rect4> months_cells;
		months_cells.resize(store_size);

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			//float index_float = static_cast<float>(index);
			int current_year = calendar_config.GetYear(index);
			boost::gregorian::date first_day_of_year = boost::gregorian::date(current_year, 1, 1);

			for (auto subindex = 0U; subindex < 12; ++subindex)
			{
				auto current_cell = row_frames.GetSubFrame(index, 1);

				rect4 month_cell;
				month_cell.Left(current_cell.Left() + static_cast<float>(boost::gregorian::date_period(first_day_of_year, first_day_of_year + boost::gregorian::months(subindex)).length().days()) * day_width);
				month_cell.Bottom(current_cell.Bottom());
				month_cell.Right(current_cell.Left() + static_cast<float>(boost::gregorian::date_period(first_day_of_year, first_day_of_year + boost::gregorian::months(subindex + 1)).length().days()) * day_width);
				month_cell.Top(current_cell.Top());

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
		int number_days_cells = 0;

		number_days_cells = calendar_config.GetSpanLengthDays();

		std::vector<rect4> days_cells(number_days_cells);
		std::vector<float> days_cells_shape_linewidths(number_days_cells);
		std::vector<vec4> days_cells_shape_fillcolors(number_days_cells);
		std::vector<vec4> days_cells_shape_outlinecolors(number_days_cells);

		auto config = shape_configuration_storage.GetShapeConfiguration("Day Shapes");
		auto sunday_config = shape_configuration_storage.GetShapeConfiguration("Sunday Shapes");

		for (int index = 0; index < calendar_config.GetSpanLengthYears(); ++index)
		{
			int current_year = calendar_config.GetYear(index);
			int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();

			for (int subindex = 0; subindex < number_days; ++subindex)
			{
				float float_subindex = static_cast<float>(subindex);

				auto current_cell = row_frames.GetSubFrame(index, 1);

				rect4 day_cell;
				day_cell.Left(current_cell.Left() + float_subindex * day_width);
				day_cell.Right(day_cell.Left() + day_width);
				day_cell.Bottom(current_cell.Bottom());
				day_cell.Top(current_cell.Top());

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

		std::vector<rect4> bars_cells;
		std::vector<float> bars_cells_shape_linewidths;
		std::vector<vec4> bars_cells_shape_fillcolors;
		std::vector<vec4> bars_cells_shape_outlinecolors;

		bars_cells.reserve(number_bars);
		bars_cells_shape_linewidths.reserve(number_bars);
		bars_cells_shape_fillcolors.reserve(number_bars);
		bars_cells_shape_outlinecolors.reserve(number_bars);

		graphic_engine->RemoveShapes(bar_labels_text);
		bar_labels_text.clear();

		for (size_t index = 0; index < data_store.GetNumberBars(); ++index)
		{
			if (calendar_config.IsInSpan(data_store.GetBar(index).GetYear()))
			{
				auto current_group = data_store.GetBar(index).group;

				auto search_string = std::wstring(L"Bar Group ") + std::to_string(current_group);
				auto current_shape_config = shape_configuration_storage.GetShapeConfiguration(search_string);

				bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
				bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
				bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());

				int row = data_store.GetBar(index).GetYear() - calendar_config.GetSpanLimitsYears()[0];

				auto current_sub_cell = row_frames.GetSubFrame(row, 1);

				rect4 bar_cell;
				bar_cell.Left(current_sub_cell.Left() + data_store.GetBar(index).GetFirstDay() * day_width);
				bar_cell.Bottom(current_sub_cell.Bottom());
				bar_cell.Right(current_sub_cell.Left() + data_store.GetBar(index).GetLastDay() * day_width);
				bar_cell.Top(current_sub_cell.Top());

				bars_cells.push_back(bar_cell);

				std::string numsText = data_store.GetBar(index).GetText();

				bar_labels_text.push_back(graphic_engine->AddShape<FontShape>());
				bar_labels_text.back()->SetFont(font_loader);

				auto current_text_cell = row_frames.GetSubFrame(row, 2);

				current_text_cell.Left(bar_cell.Left());
				current_text_cell.Right(bar_cell.Right());

				bar_labels_text.back()->SetShapeCentered(numsText, current_text_cell.Center(), current_text_cell.Height());
			}
		}

		bars_cells.shrink_to_fit();
		bars_cells_shape_linewidths.shrink_to_fit();
		bars_cells_shape_fillcolors.shrink_to_fit();
		bars_cells_shape_outlinecolors.shrink_to_fit();

		bars_cells_shape->SetShapes(bars_cells, bars_cells_shape_linewidths, bars_cells_shape_fillcolors, bars_cells_shape_outlinecolors);
	}

	void SetupYearsTotals()
	{
		graphic_engine->RemoveShapes(years_totals_text);
		years_totals_text.clear();

		std::vector<rect4> years_totals_cells;
		years_totals_cells.resize(data_store.GetSpan());

		for (size_t index = 0; index < data_store.GetSpan(); ++index)
		{
			if (calendar_config.IsInSpan(data_store.GetFirstYear() + index))
			{
				int row = data_store.GetFirstYear() + index - calendar_config.GetSpanLimitsYears()[0];

				auto current_cell = row_frames.GetSubFrame(row, 0);

				rect4 year_total_cell = current_cell;
				year_total_cell.Right(current_cell.Left() + static_cast<float>(data_store.GetAnnualTotal(index)) * day_width);
				years_totals_cells[index] = year_total_cell;

				auto current_year = data_store.GetFirstYear() + index;
				int number_days = boost::gregorian::date_period(boost::gregorian::date(current_year, 1, 1), boost::gregorian::date(current_year + 1, 1, 1)).length().days();

				float percent = static_cast<float>(data_store.GetAnnualTotal(index)) / static_cast<float>(number_days);

				std::array<char, 100> year_total_text_buffer;
				auto text_lenght = snprintf(year_total_text_buffer.data(), year_total_text_buffer.size(), "%.*f %%", 1, percent * 100.0f);
				auto year_total_text = std::string(year_total_text_buffer.data());
				auto year_total_text_width = font_loader->TextWidth(year_total_text, year_total_cell.Height());

				rect4 year_total_text_cell;
				year_total_text_cell.Left(year_total_cell.Right() + current_cell.Height());
				year_total_text_cell.Bottom(year_total_cell.Bottom());
				year_total_text_cell.Right(year_total_text_cell.Left() + year_total_text_width);
				year_total_text_cell.Top(year_total_cell.Top());

				years_totals_text.push_back(graphic_engine->AddShape<FontShape>());
				years_totals_text.back()->SetFont(font_loader);
				years_totals_text.back()->SetShapeCentered(year_total_text, year_total_text_cell.Center(), year_total_text_cell.Height());
			}
		}

		auto config = shape_configuration_storage.GetShapeConfiguration("Years Totals");

		years_totals_shape->SetShapes(years_totals_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
	}

	void SetupLegend()
	{
		size_t number_entrie_frames = (date_group_store.GetDateGroups().size() + 1/*annual_total*/) * 2;

		std::vector<rect4> legend_entries_frames(number_entrie_frames);

		auto entries_width = legend_frame.Width() / static_cast<float>(number_entrie_frames);

		for (size_t index = 0; index < number_entrie_frames; ++index)
		{
			auto float_index = static_cast<float>(index);
			legend_entries_frames[index] = legend_frame;
			legend_entries_frames[index].Left(legend_frame.Left() + entries_width * float_index);
			legend_entries_frames[index].Right(legend_frame.Left() + entries_width * float_index + entries_width);
		}

		std::vector<rect4> bars_cells;
		std::vector<float> bars_cells_shape_linewidths;
		std::vector<vec4> bars_cells_shape_fillcolors;
		std::vector<vec4> bars_cells_shape_outlinecolors;

		// find max lenght string
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

		if (font_loader)
		{
			auto legend_font_size = font_loader->AdjustTextSize(legend_entries_frames[0], string_max_lenght, 0.5f, 0.75f);

			graphic_engine->RemoveShapes(legend_text);
			legend_text.clear();

			for (size_t index = 0; index < date_group_store.GetDateGroups().size(); ++index)
			{
				legend_text.push_back(graphic_engine->AddShape<FontShape>());
				legend_text.back()->SetFont(font_loader);
				legend_text.back()->SetShapeCentered(date_group_store.GetDateGroups()[index].name, legend_entries_frames[index * 2].Center(), legend_font_size);

				//if (calendar_config.GetSpanLengthYears() > 0 && (bar_shape_configs.size() == date_group_store.GetDateGroups().size()))
				if (calendar_config.GetSpanLengthYears() > 0)
				{
					auto current_height = row_frames.GetSubFrame(0, 1).Height();

					auto current_cell = legend_entries_frames[index * 2 + 1];
					auto current_vertical_center = current_cell.Center().y;
					current_cell.Bottom(current_vertical_center - current_height / 2.f);
					current_cell.Top(current_vertical_center + current_height / 2.f);

					bars_cells.push_back(current_cell);

					auto search_string = std::wstring(L"Bar Group ") + std::to_string(index);
					auto current_shape_config = shape_configuration_storage.GetShapeConfiguration(search_string);

					bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
					bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
					bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());
				}
			}

			legend_text.push_back(graphic_engine->AddShape<FontShape>());
			legend_text.back()->SetFont(font_loader);
			legend_text.back()->SetShapeCentered("Annual Sums", legend_entries_frames[legend_entries_frames.size() - 2].Center(), legend_font_size);

			if (calendar_config.GetSpanLengthYears() > 0)
			{
				auto current_height = row_frames.GetSubFrame(0, 0).Height();

				auto current_cell = legend_entries_frames[legend_entries_frames.size() - 1];
				auto current_vertical_center = current_cell.Center().y;
				current_cell.Bottom(current_vertical_center - current_height / 2.f);
				current_cell.Top(current_vertical_center + current_height / 2.f);

				bars_cells.push_back(current_cell);

				auto current_shape_config = shape_configuration_storage.GetShapeConfiguration("Years Totals");

				bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
				bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
				bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());
			}

		}

		legend_entries_shape->SetShapes(bars_cells, bars_cells_shape_linewidths, bars_cells_shape_fillcolors, bars_cells_shape_outlinecolors);
		//legend_shape->SetShapes(legend_entries_frames, 0.15f, vec4(1.f, 1.f, 1.f, 0.f), vec4(0.f, 0.f, 0.f, 1.f));
	}

private:

	GraphicEngine* graphic_engine;

	DateIntervalBundleBarStore data_store;
	CalendarConfigStorage calendar_config;
	
	RowFrames row_frames;
	
	rect4 page_size;
	rect4 page_margin;
	rect4 print_area;
	
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

	TitleConfig title_config;

	float labels_font_size;

	////////////////////////////////////////////////////////////////////////////////

	ShapeConfigurationStorage shape_configuration_storage;

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

