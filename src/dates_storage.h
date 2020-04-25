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


#ifndef DATES_H
#define DATES_H


#define BOOST_DATE_TIME_NO_LIB
#include <boost/date_time/gregorian/gregorian.hpp>

using date = boost::gregorian::date;
using date_duration = boost::gregorian::date_duration;
using date_period = boost::gregorian::date_period;
using months = boost::gregorian::months;


#include <sigslot/signal.hpp>

#include <fstream>
#include <sstream>
#include <array>

#include <pugixml.hpp>


class Bar
{
public:

	Bar(const date_period& date_interval);
	
	void SetNumber(size_t number);
	int GetYear() const;
	int GetLenght() const;
	float GetFirstDay() const;
	float GetLastDay() const;
	size_t GetNumber() const;

private:

	date_period date_interval;
	size_t number;
	int admission;
	int lenght;
};






class DateIntervals
{
public:

	DateIntervals();

	virtual void SetDateIntervals(const std::vector<date_period>& date_intervals);
	
	void LoadXML(const pugi::xml_node& doc);
	void SaveXML(pugi::xml_node* doc);

	size_t GetDateIntervalsSize() const;
	const date_period& GetDateIntervalConstRef(size_t index) const;
	const date_period& GetDateInterIntervalConstRef(size_t index) const;

	int GetSpan() const;
	int GetFirstYear() const;
	int GetLastYear() const;

	static bool CheckDateInterval(const date& begin_date, const date& end_date);

	void SetTransform(int shift_begin_date, int shift_end_date);

	sigslot::signal<const std::vector<date_period>&> signal_transformed_date_intervals;
	sigslot::signal<const std::vector<date_period>&, const std::vector<date_period>&> signal_date_intervals;

protected:

	void ProcessDateIntervals(const std::vector<date_period>& date_intervals);
	void Sort();
	void ProcessDateInterIntervals();
	void ProcessShiftDateIntervals();
	void SendDateIntervals();

	std::vector<date_period> date_intervals;
	std::vector<date_period> date_inter_intervals;
	std::vector<date_period> shifted_date_intervals;

	std::array<int, 2> date_shift;
};


class DateIntervalStore : public DateIntervals
{
public:

	void SetDateIntervals(const std::vector<date_period>& date_intervals) override;

	size_t GetNumberBars() const;
	Bar GetBar(size_t index) const;
	int GetAnnualTotal(size_t index) const;

private:

	void ProcessBars();
	void ProcessAnnualTotals();

	std::vector<Bar> bars;
	std::vector<int> annualTotals;
};


#endif /* DATES_H */
