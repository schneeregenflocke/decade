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

/*DateCSV::DateCSV()
	: seperator(L'\t'), isOpen(false)
{}

bool DateCSV::OpenFile(const std::string& fileName)
{
	fileStream.open(fileName);
	if (fileStream.is_open())
	{
		isOpen = true;
	}
	return isOpen;
}

void DateCSV::CloseFile()
{
	fileStream.close();
	if (!fileStream.is_open())
	{
		isOpen = false;
	}
}

void DateCSV::SetSeperator(char argSeperator)
{
	seperator = argSeperator;
}

std::vector<date_period> DateCSV::readFile()
{
	std::vector<date_period> date_intervals;

	std::string line;
	while (std::getline(fileStream, line))
	{
		std::stringstream lineStream(line);
		std::string column;

		std::array<date, 2> datePeriodParts;

		for (size_t index = 0; index < 2; ++index)
		{
			std::getline(lineStream, column, seperator);
			std::stringstream dateStream(column);
			std::string datePart;

			std::array<int, 3> dateParts;

			for (size_t subIndex = 0; subIndex < 3; ++subIndex)
			{
				std::getline(dateStream, datePart, '.');
				dateParts[subIndex] = std::atoi(datePart.c_str());
			}

			try
			{
				datePeriodParts[index] = date(dateParts[2], dateParts[1], dateParts[0]);
			}
			catch (const std::exception & excptn)
			{
				datePeriodParts[index] = date(boost::date_time::not_a_date_time);
			}
		}
		
		if ((datePeriodParts[0].is_not_a_date() || datePeriodParts[1].is_not_a_date()) == false)
		{
			date_period currentDatePeriod(datePeriodParts[0], datePeriodParts[1] + date_duration(1));

			if (!currentDatePeriod.is_null())
			{
				date_intervals.push_back(currentDatePeriod);
			}
		}	
	}

	return date_intervals;
}*/




void DateIntervals::SetDateIntervals(const std::vector<date_period>& values)
{
	date_intervals.clear();

	date_intervals.reserve(values.size());

	for (const auto& period : values)
	{
		if (CheckDateInterval(period.begin(), period.end()))
		{
			date_intervals.push_back(period);
		}
	}

	date_intervals.shrink_to_fit();
	
	Sort();
	ProcessDateInterIntervals();

	ProcessBars();
	ProcessAnnualTotals();
}

bool DateIntervals::CheckDateInterval(date begin_date, date end_date)
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


/*const std::vector<date_period>& DateIntervals::GetDateIntervalsConstRef() const
{
	return date_intervals;
}*/

/*const std::vector<date_period>& DateIntervals::GetDateInterIntervalsConstRef() const
{
	return date_inter_intervals;
}*/

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

void DateIntervals::ProcessBars()
{
	bars.clear();

	for(size_t index = 0; index < date_intervals.size(); ++index)
	{
		int span = date_intervals[index].last().year() - date_intervals[index].begin().year();

		std::vector<boost::gregorian::date_period> splitDatePeriods;
		splitDatePeriods.push_back(date_intervals[index]);

		for (auto subIndex = 0; subIndex < span; ++subIndex)
		{
			boost::gregorian::date split_date = boost::gregorian::date(splitDatePeriods[subIndex].begin().year() + 1, 1, 1);
			splitDatePeriods.push_back(boost::gregorian::date_period(split_date, splitDatePeriods[subIndex].end()));
			splitDatePeriods[subIndex] = boost::gregorian::date_period(splitDatePeriods[subIndex].begin(), split_date);
		}

		for (auto subindex = 0U; subindex < splitDatePeriods.size(); ++subindex)
		{
			Bar bar(splitDatePeriods[subindex]);
			bar.setNumber(index);
			bars.push_back(bar);
		}	
	}
}

void DateIntervals::ProcessAnnualTotals()
{
	annualTotals.clear();
	annualTotals.resize(GetSpan());
	
	for (size_t index = 0; index < bars.size(); ++index)
	{
		size_t annualTotalsIndex = static_cast<size_t>(bars[index].getYear()) - static_cast<size_t>(GetFirstYear());

		annualTotals[annualTotalsIndex] += bars[index].getLenght();
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

size_t DateIntervals::GetNumberBars() const
{
	return bars.size();
}

Bar DateIntervals::GetBar(size_t index) const
{
	return bars[index];
}

int DateIntervals::GetAnnualTotal(size_t index) const
{
	return annualTotals[index];
}



Bar::Bar(const date_period& date_interval) : 
	dateInterval(date_interval)
{
}

void Bar::setNumber(int argNumber)
{
	number = argNumber;
}

int Bar::getYear()
{
	return dateInterval.begin().year();
}

int Bar::getLenght()
{
	return dateInterval.length().days();
}

float Bar::getFirstDay()
{
	return static_cast<float>(dateInterval.begin().day_of_year() - 1);
}

float Bar::getLastDay()
{
	return static_cast<float>(dateInterval.last().day_of_year());
}

int Bar::getNumber()
{
	return number;
}


