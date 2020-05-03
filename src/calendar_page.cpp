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

#include "calendar_page.h"

void CalendarSpan::SetSpan(int first_year, int last_year)
{
	first = first_year;
	last = last_year;
}

bool CalendarSpan::IsInSpan(int year)
{
	bool is_in_span = false;
	if (year >= first && year <= last)
	{
		is_in_span = true;
	}
	return is_in_span;
}

int CalendarSpan::GetFirstYear()
{
	return first;
}

int CalendarSpan::GetLastYear()
{
	return last;
}

int CalendarSpan::GetSpan()
{
	return last - first + 1;
}


std::vector<float> Section(const std::vector<float>& weights, float value)
{
	auto sum = std::accumulate(weights.cbegin(), weights.cend(), 0.f);
	
	std::vector<float> sections;

	for (const auto& weight : weights)
	{
		sections.push_back((weight / sum) * value);
	}

	return sections;
}



void RowFrames::SetupRowFrames(const rect4& main_frame, size_t num_row_frames)
{
	row_frames.resize(num_row_frames);
	
	auto row_height = main_frame.Height() / static_cast<float>(num_row_frames);

	for (size_t index = 0; index < row_frames.size(); ++index)
	{
		auto float_index = static_cast<float>(index);
		auto current_bottom = main_frame.Bottom() + float_index * row_height;
		row_frames[index].Left(main_frame.Left());
		row_frames[index].Bottom(current_bottom);
		row_frames[index].Right(main_frame.Right());
		row_frames[index].Top(current_bottom + row_height);
	}
}



void RowFrames::SetupSubFrames(std::vector<float> weights, float gap_factor)
{
	number_sub_frames = weights.size();
	sub_frames.resize(row_frames.size() * weights.size());

	for (size_t index = 0; index < row_frames.size(); ++index)
	{
		auto sections = Section(weights, row_frames[index].Height());

		std::vector<float> cumulative_sections(sections.size() + 1);
		cumulative_sections[0] = 0.f;
		for (size_t subindex = 1; subindex < cumulative_sections.size(); ++subindex)
		{
			cumulative_sections[subindex] = std::accumulate(sections.cbegin(), sections.cbegin() + subindex, 0.f);
		}

		for (size_t subindex = 0; subindex < sections.size(); ++subindex)
		{
			auto gap = sections[subindex] * gap_factor;

			sub_frames[index * weights.size() + subindex].Left(row_frames[index].Left());
			sub_frames[index * weights.size() + subindex].Bottom(row_frames[index].Bottom() + cumulative_sections[subindex] + gap);
			sub_frames[index * weights.size() + subindex].Right(row_frames[index].Right());
			sub_frames[index * weights.size() + subindex].Top(row_frames[index].Bottom() + cumulative_sections[subindex + 1] - gap);
		}
	}
}

rect4 RowFrames::GetSubFrame(size_t row, size_t sub)
{
	return sub_frames[number_sub_frames * row + sub];
}

std::vector<rect4> RowFrames::GetSubFrames()
{
	return sub_frames;
}



CalendarPage::CalendarPage(GraphicEngine* graphic_engine) : 
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

void CalendarPage::SetDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	data_store.SetDateIntervalBundles(date_interval_bundles);

	Update();
}

void CalendarPage::SlotPageSize(const std::array<float, 2>& page_size)
{
	this->page_size = rect4(page_size[0], page_size[1]);

	Update();
}

void CalendarPage::SlotPageMargins(const std::array<float, 4>& page_margins)
{
	page_margin = rect4(page_margins[0], page_margins[1], page_margins[2], page_margins[3]);
	
	Update();
}

void CalendarPage::SlotSelectFont(const std::string& font_path)
{
	font.reset(std::make_unique<FontLoader>(font_path).release());

	Update();
}

void CalendarPage::SlotTitleFrameHeight(float height)
{
	title_frame_height = height;

	Update();
}

void CalendarPage::SlotTitleFontSizeRatio(float ratio)
{
	title_font_size_ratio = ratio;

	Update();
}

void CalendarPage::SlotTitleText(const std::wstring& text)
{
	title_text = text;

	Update();
}

void CalendarPage::SlotTitleTextColor(const std::array<float, 4>& title_text_color)
{
	this->title_text_color = glm::vec4(title_text_color[0], title_text_color[1], title_text_color[2], title_text_color[3]);

	Update();
}

void CalendarPage::SlotRectangleShapeConfig(const std::vector<RectangleShapeConfig>& configs)
{
	element_configurations = configs;
	
	bar_shape_configs.clear();
	bool run_loop = true;
	size_t index = 0;
	while(run_loop)
	{
		auto search_string = std::wstring(L"Bar Group ") + std::to_string(index);
		
		auto shape_config = GetShapeConfig(search_string);

		if (shape_config.Name() != L"Not Found")
		{
			bar_shape_configs.push_back(shape_config);
		}
		else
		{
			run_loop = false;
		}
		++index;
	} 
	
	Update();
}

void CalendarPage::SlotCalendarConfig(const CalendarConfig& config)
{
	calendar_config = config;

	Update();
}


void CalendarPage::Update()
{
	rect4 view_size = page_size.Scale(1.1f);
	graphic_engine->GetViewRef().SetOrthoMatrix(view_size);

	glm::vec4 page_shape_color = vec4(1.f, 1.f, 1.f, 1.f);
	page_shape->SetShape(page_size, page_shape_color);

	print_area = page_size.Reduce(page_margin);

	title_frame = print_area;
	title_frame.Bottom(title_frame.Top() - title_frame_height);

	
	page_margin_frame = print_area;
	page_margin_frame.Top(title_frame.Bottom());
	rect4 page_margin_frame_margin = rect4(0.f, 0.f, 0.f, 5.f);
	page_margin_frame = page_margin_frame.Reduce(page_margin_frame_margin);


	calendar_frame = page_margin_frame;
	rect4 calendar_frame_margin = rect4(0.f, 0.f, 3.f, 2.f);
	calendar_frame = calendar_frame.Reduce(calendar_frame_margin);


////////////////////////////////////////
	if (calendar_config.auto_calendar_range == true && data_store.GetSpan() > 0)
	{
		calendarSpan.SetSpan(data_store.GetFirstYear(), data_store.GetLastYear());
	}
	else
	{
		calendarSpan.SetSpan(0, -1);
	}
	
	if (calendar_config.auto_calendar_range == false)
	{
		int config_range = calendar_config.max_calendar_range - calendar_config.min_calendar_range;
		if (config_range >= 0)
		{
			calendarSpan.SetSpan(calendar_config.min_calendar_range, calendar_config.max_calendar_range);
		}
		else
		{
			calendarSpan.SetSpan(0, -1);
		}
	}
////////////////////////////////////////	


	size_t number_rows = calendarSpan.GetSpan() + 2;

	cell_width = calendar_frame.Width() / 13.f;
	row_height = calendar_frame.Height() / static_cast<float>(number_rows);

	cells_frame = calendar_frame.Reduce(rect4(cell_width, row_height * 2.f, 0.f, 0.f));

	row_frames.SetupRowFrames(cells_frame, calendarSpan.GetSpan());
	row_frames.SetupSubFrames(calendar_config.sub_row_weights, calendar_config.gap_factor);
	
	day_width = cells_frame.Width() / 366.f;

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
	
//////////////////////////////////////

	SetLegend();

	graphic_engine->Refresh();
}


void CalendarPage::SetupPrintAreaShape()
{
	auto config = GetShapeConfig(L"Page Margin");
	
	print_area_shape->SetShapes(page_margin_frame, config.LineWidth(), config.FillColor(), config.OutlineColor());
}

void CalendarPage::SetupTitleShape()
{
	auto config = GetShapeConfig(L"Title Frame");

	title_area_shape->SetShapes(title_frame, config.LineWidth(), config.FillColor(), config.OutlineColor());

	title_font_shape->SetFont(font.get());
	title_font_shape->SetShapeHVCentered(title_text, title_frame.Center(), title_frame.Height() * title_font_size_ratio);
}


void CalendarPage::SetupCalendarLabelsShape()
{
	wchar_t buf[100];
	std::wstring format = L"%b";

	std::array<std::wstring, 12> months_names;
	for (size_t index = 0; index < months_names.size(); ++index)
	{
		std::tm month_tm = { 0 };
		month_tm.tm_mon = index;
		auto len = std::wcsftime(buf, sizeof(buf), format.c_str(), &month_tm);

		months_names[index] = buf;
	}
	

	std::vector<rect4> x_label_frames(12);

	for (size_t index = 0; index < months_names.size(); ++index)
	{
		float floatIndex = static_cast<float>(index);

		//float cell_xcenter = x_labels_frame.Left() + cell_width * floatIndex + cell_width / 2.f;
		//float cell_ycenter = x_labels_frame.Bottom() + row_height / 2.f;

		x_label_frames[index].Left(x_labels_frame.Left() + cell_width * floatIndex);
		x_label_frames[index].Bottom(x_labels_frame.Bottom());
		x_label_frames[index].Right(x_labels_frame.Left() + cell_width * floatIndex + cell_width);
		x_label_frames[index].Top(x_labels_frame.Top());

		labels_font_size = row_height * .5f;

		month_label_text[index]->SetFont(font.get());
		

		//	provisional (better than nothing)
		if (font)
		{
			auto text_width = month_label_text[index]->TextWidth(L"00000", labels_font_size);
			auto hw_ratio = labels_font_size / text_width;

			if (text_width > cell_width * 0.75f)
			{
				//auto overlapping_ratio = cell_width / text_width;
				
				labels_font_size = hw_ratio * cell_width * 0.75f;
			}
		}

		month_label_text[index]->SetShapeHVCentered(months_names[index], x_label_frames[index].Center(), labels_font_size);
	}

	auto config = GetShapeConfig(L"Calendar Labels");

	column_labels_shape->SetShapes(x_label_frames, config.LineWidth(), config.FillColor(), config.OutlineColor());


	graphic_engine->RemoveShapes(annual_labels_text);
	annual_labels_text = graphic_engine->AddShapes<FontShape>(calendarSpan.GetSpan());

	std::vector<rect4> y_labels_rows(calendarSpan.GetSpan());
	for (int index = 0; index < calendarSpan.GetSpan(); ++index)
	{
		float floatIndex = static_cast<float>(index);

		y_labels_rows[index] = rect4(
			y_labels_frame.Left(),
			y_labels_frame.Bottom() + row_height * floatIndex,
			y_labels_frame.Right(),
			y_labels_frame.Bottom() + row_height * floatIndex + row_height
		);

		std::wstring current_year_text = std::to_wstring(calendarSpan.GetFirstYear() + index);

		annual_labels_text[index]->SetFont(font.get());
		annual_labels_text[index]->SetShapeHVCentered(current_year_text, y_labels_rows[index].Center(), labels_font_size);
	}

	row_labels_shape->SetShapes(y_labels_rows, config.LineWidth(), config.FillColor(), config.OutlineColor());
}


void CalendarPage::SetupYearsShapes()
{
	
	
	std::vector<rect4> years_cells;
	years_cells.resize(calendarSpan.GetSpan());
	
	for (int index = 0; index < calendarSpan.GetSpan(); ++index)
	{
		float indexfloat = static_cast<float>(index);
		int current_year = calendarSpan.GetFirstYear() + index;
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

	auto config = GetShapeConfig(L"Years Shapes");

	years_cells_shape->SetShapes(years_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
}



void CalendarPage::SetupMonthsShapes()
{
	size_t store_size = calendarSpan.GetSpan() * 12;
	std::vector<rect4> months_cells;
	months_cells.resize(store_size);

	for (int index = 0; index < calendarSpan.GetSpan(); ++index)
	{
		float indexfloat = static_cast<float>(index);
		int current_year = calendarSpan.GetFirstYear() + index;
		boost::gregorian::date first_day_of_year = boost::gregorian::date(current_year, 1, 1);

		for (auto subindex = 0U; subindex < 12; ++subindex)
		{
			auto current_cell = row_frames.GetSubFrame(index, 1);
			
			rect4 month_cell;
			month_cell.Left(current_cell.Left() + static_cast<float>(date_period(first_day_of_year, first_day_of_year + months(subindex)).length().days())* day_width);
			month_cell.Bottom(current_cell.Bottom());
			month_cell.Right(current_cell.Left() + static_cast<float>(boost::gregorian::date_period(first_day_of_year, first_day_of_year + boost::gregorian::months(subindex + 1)).length().days())* day_width);
			month_cell.Top(current_cell.Top());

			size_t store_index = index * 12 + subindex;
			
			months_cells[store_index] = month_cell;
		}
	}

	auto config = GetShapeConfig(L"Months Shapes");

	months_cells_shape->SetShapes(months_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
}


void CalendarPage::SetupDaysShapes()
{
	using namespace boost::gregorian;

	int days_index = 0;
	int number_days_cells = 0;

	if (calendarSpan.GetSpan() > 0)
	{
		number_days_cells = date_period(date(calendarSpan.GetFirstYear(), 1, 1), date(calendarSpan.GetLastYear() + 1, 1, 1)).length().days();
	}

	std::vector<rect4> days_cells(number_days_cells);
	std::vector<float> days_cells_shape_linewidths(number_days_cells);
	std::vector<vec4> days_cells_shape_fillcolors(number_days_cells);
	std::vector<vec4> days_cells_shape_outlinecolors(number_days_cells);

	auto config = GetShapeConfig(L"Day Shapes");
	auto sunday_config = GetShapeConfig(L"Sunday Shapes");

	for (int index = 0; index < calendarSpan.GetSpan(); ++index)
	{
		int current_year = calendarSpan.GetFirstYear() + index;
		int number_days = date_period(date(current_year, 1, 1), date(current_year + 1, 1, 1)).length().days();

		for (int subindex = 0; subindex < number_days; ++subindex)
		{
			float float_subindex = static_cast<float>(subindex);

			auto current_cell = row_frames.GetSubFrame(index, 1);

			rect4 day_cell;
			day_cell.Left(current_cell.Left() + float_subindex * day_width);
			day_cell.Bottom(current_cell.Bottom());
			day_cell.Right(day_cell.Left() + day_width);
			day_cell.Top(current_cell.Top());
			
			days_cells[days_index] = day_cell;

			date current_date = date(calendarSpan.GetFirstYear(), 1, 1) + date_duration(days_index);
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

void CalendarPage::SetupBarsShape()
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
		if (calendarSpan.IsInSpan(data_store.GetBar(index).GetYear()))
		{
			auto current_group = data_store.GetBar(index).group;
			auto current_shape_config = bar_shape_configs[current_group];

			bars_cells_shape_linewidths.push_back(current_shape_config.LineWidth());
			bars_cells_shape_fillcolors.push_back(current_shape_config.FillColor());
			bars_cells_shape_outlinecolors.push_back(current_shape_config.OutlineColor());

			int row = data_store.GetBar(index).GetYear() - calendarSpan.GetFirstYear();

			auto current_sub_cell = row_frames.GetSubFrame(row, 1);

			rect4 bar_cell;
			bar_cell.Left(current_sub_cell.Left() + data_store.GetBar(index).GetFirstDay() * day_width);
			bar_cell.Bottom(current_sub_cell.Bottom());
			bar_cell.Right(current_sub_cell.Left() + data_store.GetBar(index).GetLastDay() * day_width);
			bar_cell.Top(current_sub_cell.Top());

			bars_cells.push_back(bar_cell);

			std::wstring numsText = data_store.GetBar(index).GetText();

			bar_labels_text.push_back(graphic_engine->AddShape<FontShape>());
			bar_labels_text.back()->SetFont(font.get());

			auto current_text_cell = row_frames.GetSubFrame(row, 2);

			current_text_cell.Left(bar_cell.Left());
			current_text_cell.Right(bar_cell.Right());

			bar_labels_text[index]->SetShapeHVCentered(numsText, current_text_cell.Center(), current_text_cell.Height());
		}
	}

	bars_cells.shrink_to_fit();
	bars_cells_shape_linewidths.shrink_to_fit();
	bars_cells_shape_fillcolors.shrink_to_fit();
	bars_cells_shape_outlinecolors.shrink_to_fit();

	bars_cells_shape->SetShapes(bars_cells, bars_cells_shape_linewidths, bars_cells_shape_fillcolors, bars_cells_shape_outlinecolors);
}



void CalendarPage::SetupYearsTotals()
{
	std::vector<rect4> years_totals_cells;
	years_totals_cells.resize(data_store.GetSpan());

	for (size_t index = 0; index < data_store.GetSpan(); ++index)
	{
		//int row = data_store->GetBar(index).GetYear() - calendarSpan.GetFirstYear();

		if (calendarSpan.IsInSpan(data_store.GetFirstYear() + index))
		{
			int row = data_store.GetFirstYear() + index - calendarSpan.GetFirstYear();

			auto current_cell = row_frames.GetSubFrame(row, 0);

			rect4 year_total_cell;
			year_total_cell.Left(current_cell.Left());
			year_total_cell.Bottom(current_cell.Bottom());
			year_total_cell.Right(current_cell.Left() + static_cast<float>(data_store.GetAnnualTotal(index)) * day_width);
			year_total_cell.Top(current_cell.Top());

			years_totals_cells[index] = year_total_cell;

		}
	}

	auto config = GetShapeConfig(L"Years Totals");

	years_totals_shape->SetShapes(years_totals_cells, config.LineWidth(), config.FillColor(), config.OutlineColor());
}

void CalendarPage::SetLegend()
{
	legend_shape->SetShapes(legend_frame, 0.15f, vec4(1.f, 1.f, 1.f, 0.f), vec4(0.f, 0.f, 0.f, 1.f));

	
	size_t number_entries = 2;
	std::vector<rect4> legend_entries_frames(number_entries);

	auto entries_width = legend_frame.Width() / static_cast<float>(number_entries);

	for (size_t index = 0; index < number_entries; ++index)
	{
		auto floatIndex = static_cast<float>(index);
		legend_entries_frames[index] = legend_frame;
		legend_entries_frames[index].Left(legend_frame.Left() + entries_width * floatIndex);
		legend_entries_frames[index].Right(legend_frame.Left() + entries_width * floatIndex + entries_width);
	}

	legend_shape->SetShapes(legend_entries_frames, 0.15f, vec4(1.f, 1.f, 1.f, 0.f), vec4(0.f, 0.f, 0.f, 1.f));
}

RectangleShapeConfig CalendarPage::GetShapeConfig(const std::wstring& name)
{
	auto found = std::find(element_configurations.begin(), element_configurations.end(), name);
	if (found != element_configurations.end())
	{
		return *found;
	}
	
	return RectangleShapeConfig(L"Not Found");
}


