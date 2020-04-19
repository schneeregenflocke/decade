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
#include <boost\date_time\gregorian\gregorian.hpp>

using date = boost::gregorian::date;
using date_duration = boost::gregorian::date_duration;
using date_period = boost::gregorian::date_period;
using months = boost::gregorian::months;


#include <fstream>
#include <sstream>
#include <array>


/*class DateCSV
{
public:

	DateCSV();

	bool OpenFile(const std::string& fileName);
	void CloseFile();
	void SetSeperator(char argSeperator);
	std::vector<boost::gregorian::date_period> readFile();

private:
	std::ifstream fileStream;
	bool isOpen;
	char seperator;
};*/


class Bar
{

public:

	Bar(const boost::gregorian::date_period& date_interval);
	
	void setNumber(int argNumber);
	int getYear();
	int getLenght();
	float getFirstDay();
	float getLastDay();
	int getNumber();

private:

	date_period dateInterval;
	int number;
	int admission;
	int lenght;
};




class DateIntervals
{
public:

	void SetDateIntervals(const std::vector<date_period>& values);

	//const std::vector<date_period>& GetDateIntervalsConstRef() const;
	//const std::vector<date_period>& GetDateInterIntervalsConstRef() const;

	size_t GetDateIntervalsSize() const;
	const date_period& GetDateIntervalConstRef(size_t index) const;
	const date_period& GetDateInterIntervalConstRef(size_t index) const;
	
	int GetSpan() const;
	int GetFirstYear() const;
	int GetLastYear() const;
	
	size_t GetNumberBars() const;
	Bar GetBar(size_t index) const;
	int GetAnnualTotal(size_t index) const;

	static bool CheckDateInterval(date begin_date, date end_date);

private:

	void Sort();
	void ProcessDateInterIntervals();
	void ProcessBars();
	void ProcessAnnualTotals();

	

	std::vector<boost::gregorian::date_period> date_intervals;
	std::vector<boost::gregorian::date_period> date_inter_intervals;
	
	std::vector<Bar> bars;
	std::vector<int> annualTotals;
};


#endif /* DATES_H */
