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

#include "date_utils.h"
#include "date_group_store.h"

#include <sigslot/signal.hpp>

#include <fstream>
#include <sstream>
#include <array>
#include <map>

#include <pugixml.hpp>



struct DateIntervalBundle
{
	DateIntervalBundle() :
		date_interval(boost::gregorian::date_period(boost::gregorian::date(boost::date_time::not_a_date_time), boost::gregorian::date(boost::date_time::not_a_date_time))),
		date_inter_interval(boost::gregorian::date_period(boost::gregorian::date(boost::date_time::not_a_date_time), boost::gregorian::date(boost::date_time::not_a_date_time))),
		number(0),
		group(0),
		group_number(0),
		comment(L""),
		exclude(false)
	{};

	DateIntervalBundle(boost::gregorian::date_period date_interval) :
		date_interval(date_interval),
		date_inter_interval(boost::gregorian::date_period(boost::gregorian::date(boost::date_time::not_a_date_time), boost::gregorian::date(boost::date_time::not_a_date_time))),
		number(0),
		group(0),
		group_number(0),
		comment(L""),
		exclude(false)
	{};

	boost::gregorian::date_period date_interval;
	boost::gregorian::date_period date_inter_interval;
	int number;
	int group;
	int group_number;
	bool exclude;
	std::wstring comment;
};


class TransformDateIntervalBundle
{
public:

	TransformDateIntervalBundle() : date_shift{ 0, 0 } {};

	void InputDateIntervals(const std::vector<DateIntervalBundle>& date_interval_bundles);
	void InputTransformedDateIntervals(const std::vector<DateIntervalBundle>& date_interval_bundles);
	void SetTransform(int shift_begin_date, int shift_end_date);

	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_date_interval_bundles;
	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_transformed_date_interval_bundles;

private:
	std::array<int, 2> date_shift;
};


class DateIntervalBundleStore
{
public:

	virtual void SetDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles);

	int GetSpan() const;
	int GetFirstYear() const;
	int GetLastYear() const;

	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_date_interval_bundles;

	void LoadXML(const pugi::xml_node& doc);
	void SaveXML(pugi::xml_node* doc);
	////////////////////////////////////////////////////////////
	size_t GetDateIntervalsSize() const;
	const boost::gregorian::date_period& GetDateIntervalConstRef(size_t index) const;
	const boost::gregorian::date_period& GetDateInterIntervalConstRef(size_t index) const;
	////////////////////////////////////////////////////////////

	void SetDateGroups(const std::vector<DateGroup>& argument_date_groups);

protected:

	void ProcessDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles);

	std::vector<DateIntervalBundle> date_interval_bundles;

	DateGroupStore date_group_store;

private:

	void Sort();
	void ProcessNumbers();
	void ProcessDateInterIntervals();
	void ProcessDateGroupsNumber();
	void CheckAndAdjustGroupIntegrity();
	void AdjustGroupExcludeFlag();
	
	
};


class Bar
{
public:

	Bar(const boost::gregorian::date_period& date_interval);

	void SetText(const std::wstring text);
	std::wstring GetText() const;
	
	int GetYear() const;
	int GetLenght() const;
	float GetFirstDay() const;
	float GetLastDay() const;

	int group;

private:

	boost::gregorian::date_period date_interval;
	std::wstring text;
	//int admission;
	//int lenght;
	
};


class DateIntervalBundleBarStore : public DateIntervalBundleStore
{
public:

	void SetDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles) override;

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
