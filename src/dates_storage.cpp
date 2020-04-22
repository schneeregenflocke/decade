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


#include "dates_storage.h"


Bar::Bar(const date_period& date_interval) :
	date_interval(date_interval),
	number(0),
	admission(0),
	lenght(0)
{}

void Bar::SetNumber(size_t number)
{
	this->number = number;
}

int Bar::GetYear() const
{
	return date_interval.begin().year();
}

int Bar::GetLenght() const
{
	return date_interval.length().days();
}

float Bar::GetFirstDay() const
{
	return static_cast<float>(date_interval.begin().day_of_year() - 1);
}

float Bar::GetLastDay() const
{
	return static_cast<float>(date_interval.last().day_of_year());
}

size_t Bar::GetNumber() const
{
	return number;
}


DateIntervals::DateIntervals() :
	date_shift{2, 2}
{}

void DateIntervals::SetDateIntervals(const std::vector<date_period>& date_intervals)
{
	ProcessDateIntervals(date_intervals);
}

void DateIntervals::ProcessDateIntervals(const std::vector<date_period>& date_intervals)
{
	this->date_intervals.clear();
	this->date_intervals.reserve(date_intervals.size());

	for (const auto& period : date_intervals)
	{
		if (CheckDateInterval(period.begin(), period.end()))
		{
			this->date_intervals.push_back(period);
		}
	}

	this->date_intervals.shrink_to_fit();

	Sort();
	ProcessDateInterIntervals();
	ProcessShiftDateIntervals();

	SendDateIntervals();
}

void DateIntervals::SendDateIntervals()
{
	signal_date_intervals(date_intervals, date_inter_intervals);
	signal_transformed_date_intervals(shifted_date_intervals);
}

bool DateIntervals::CheckDateInterval(const date& begin_date, const date& end_date)
{
	bool valid = false;

	if (begin_date.is_special() == false && end_date.is_special() == false)
	{
		date_period period = date_period(begin_date, end_date);

		if (period.is_null() == false)
		{
			valid = true;
		}
	}

	return valid;
}

void DateIntervals::SetTransform(int shift_begin_date, int shift_end_date)
{
	date_shift[0] = shift_begin_date;
	date_shift[1] = shift_end_date;
}

size_t DateIntervals::GetDateIntervalsSize() const
{
	return date_intervals.size();
}

const date_period& DateIntervals::GetDateIntervalConstRef(size_t index) const
{
	return date_intervals[index];
}

const date_period& DateIntervals::GetDateInterIntervalConstRef(size_t index) const
{
	return date_inter_intervals[index];
}

void DateIntervals::Sort()
{
	auto sortfunc = [](const date_period period0, date_period period1) { return (period0.begin() < period1.begin()); };
	std::sort(date_intervals.begin(), date_intervals.end(), sortfunc);
}

void DateIntervals::ProcessDateInterIntervals()
{
	date_inter_intervals.clear();

	for (size_t index = 1; index < date_intervals.size(); ++index)
	{
		date_inter_intervals.push_back(date_period(date_intervals[index - 1].end(), date_intervals[index].begin()));
	}
}

void DateIntervals::ProcessShiftDateIntervals()
{
	shifted_date_intervals.clear();
	shifted_date_intervals.reserve(date_intervals.size());
	for (const auto& date_interval : date_intervals)
	{
		date_period shifted_interval = date_period(date_interval.begin() + date_duration(date_shift[0]), date_interval.end() + date_duration(date_shift[1]));
		shifted_date_intervals.emplace_back(shifted_interval);
	}
}

int DateIntervals::GetSpan() const
{
	int span = 0;
	if (date_intervals.size() > 0)
	{
		span = GetLastYear() - GetFirstYear() + 1;
	}

	return span;
}

int DateIntervals::GetFirstYear() const
{
	return date_intervals.front().begin().year();
}

int DateIntervals::GetLastYear() const
{
	return date_intervals.back().last().year();
}

void DateIntervalStore::SetDateIntervals(const std::vector<date_period>& date_intervals)
{
	ProcessDateIntervals(date_intervals);

	ProcessBars();
	ProcessAnnualTotals();
}

void DateIntervalStore::ProcessBars()
{
	bars.clear();

	for(size_t index = 0; index < date_intervals.size(); ++index)
	{
		int span = date_intervals[index].last().year() - date_intervals[index].begin().year();

		std::vector<date_period> splitDatePeriods;
		splitDatePeriods.push_back(date_intervals[index]);

		for (auto subIndex = 0; subIndex < span; ++subIndex)
		{
			date split_date = date(splitDatePeriods[subIndex].begin().year() + 1, 1, 1);
			splitDatePeriods.push_back(date_period(split_date, splitDatePeriods[subIndex].end()));
			splitDatePeriods[subIndex] = date_period(splitDatePeriods[subIndex].begin(), split_date);
		}

		for (auto subindex = 0U; subindex < splitDatePeriods.size(); ++subindex)
		{
			Bar bar(splitDatePeriods[subindex]);
			bar.SetNumber(index);
			bars.push_back(bar);
		}	
	}
}

void DateIntervalStore::ProcessAnnualTotals()
{
	annualTotals.clear();
	annualTotals.resize(GetSpan());
	
	for (size_t index = 0; index < bars.size(); ++index)
	{
		size_t annualTotalsIndex = static_cast<size_t>(bars[index].GetYear()) - static_cast<size_t>(GetFirstYear());

		annualTotals[annualTotalsIndex] += bars[index].GetLenght();
	}
}

size_t DateIntervalStore::GetNumberBars() const
{
	return bars.size();
}

Bar DateIntervalStore::GetBar(size_t index) const
{
	return bars[index];
}

int DateIntervalStore::GetAnnualTotal(size_t index) const
{
	return annualTotals[index];
}


