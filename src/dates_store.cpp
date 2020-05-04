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


#include "dates_store.h"


void TransformDateIntervalBundle::InputDateIntervals(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	std::vector<DateIntervalBundle> transformed_bundles = date_interval_bundles;
	auto bundles_iterator = transformed_bundles.begin();

	for (const auto& date_interval_bundle : date_interval_bundles) 
	{
		bundles_iterator->date_interval = date_period(
			date_interval_bundle.date_interval.begin() + date_duration(date_shift[0]), 
			date_interval_bundle.date_interval.end() + date_duration(date_shift[1]));
		++bundles_iterator;
	}

	signal_transformed_date_interval_bundles(transformed_bundles);
}

void TransformDateIntervalBundle::InputTransformedDateIntervals(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	std::vector<DateIntervalBundle> untransformed_bundles = date_interval_bundles;
	auto bundles_iterator = untransformed_bundles.begin();

	for (const auto& date_interval_bundle : date_interval_bundles)
	{
		bundles_iterator->date_interval = date_period(
			date_interval_bundle.date_interval.begin() - date_duration(date_shift[0]),
			date_interval_bundle.date_interval.end() - date_duration(date_shift[1]));
		++bundles_iterator;
	}

	signal_date_interval_bundles(untransformed_bundles);
}

void TransformDateIntervalBundle::SetTransform(int shift_begin_date, int shift_end_date)
{
	date_shift[0] = shift_begin_date;
	date_shift[1] = shift_end_date;
}


void DateIntervalBundleStore::SetDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	ProcessDateIntervalBundles(date_interval_bundles);

	signal_date_interval_bundles(this->date_interval_bundles);
}

void DateIntervalBundleStore::ProcessDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	this->date_interval_bundles.clear();
	this->date_interval_bundles.reserve(date_interval_bundles.size());

	// copy, not const reference
	for (auto date_interval_bundle : date_interval_bundles)
	{
		auto case_id = CheckAndAdjustDateInterval(&date_interval_bundle.date_interval);
		
		if (case_id > 0)
		{
			//DateIntervalBundle temporary_bundle;
			
			//temporary_bundle.date_interval = date_interval_bundle.date_interval;
			//temporary_bundle.group = date_interval_bundle.group;
			//temporary_bundle.comment = date_interval_bundle.comment;

			this->date_interval_bundles.push_back(date_interval_bundle);
		}
	}

	this->date_interval_bundles.shrink_to_fit();

	Sort();

	CheckAndAdjustGroupIntegrity();
	AdjustGroupExcludeFlag();

	ProcessNumbers();
	ProcessDateInterIntervals();
	ProcessDateGroupsNumber();
}

size_t DateIntervalBundleStore::GetDateIntervalsSize() const
{
	return date_interval_bundles.size();
}

const date_period& DateIntervalBundleStore::GetDateIntervalConstRef(size_t index) const
{
	return date_interval_bundles[index].date_interval;
}

const date_period& DateIntervalBundleStore::GetDateInterIntervalConstRef(size_t index) const
{
	return date_interval_bundles[index].date_inter_interval;
}

void DateIntervalBundleStore::SetDateGroups(const std::vector<DateGroup>& argument_date_groups)
{
	date_group_store.SetDateGroups(argument_date_groups);
}

void DateIntervalBundleStore::Sort()
{
	auto sortfunc = [](const DateIntervalBundle bundle0, DateIntervalBundle bundle1) { return (bundle0.date_interval.begin() < bundle1.date_interval.begin()); };
	std::sort(date_interval_bundles.begin(), date_interval_bundles.end(), sortfunc);
}

void DateIntervalBundleStore::ProcessNumbers()
{
	int current_number = 0;
	for (auto& date_interval_bundle : date_interval_bundles)
	{
		if (date_interval_bundle.exclude == false)
		{
			date_interval_bundle.number = current_number;
			++current_number;
		}
	}
}

void DateIntervalBundleStore::ProcessDateInterIntervals()
{
	auto iterator_first = date_interval_bundles.begin();
	int counter = 0;
	
	while (iterator_first != date_interval_bundles.end())
	{
		//std::cout << iterator_first - date_interval_bundles.begin() << '\n';

		while (iterator_first != date_interval_bundles.end() && iterator_first->exclude == true)
		{
			++iterator_first;
		}

		if (iterator_first != date_interval_bundles.end())
		{
			auto iterator_second = iterator_first + 1;

			while (iterator_second != date_interval_bundles.end() && iterator_second->exclude == true)
			{
				++iterator_second;
			}

			if (iterator_second != date_interval_bundles.end())
			{
				iterator_first->date_inter_interval = date_period(iterator_first->date_interval.end(), iterator_second->date_interval.begin());
				

				//std::cout << "wrote inter " << counter << '\n';
				++counter;
			}
		}
		++iterator_first;

		
		//std::cout << "iterator != end? "  << (iterator_first != date_interval_bundles.end()) << '\n';
	}

	/*for (size_t index = 1; index < date_interval_bundles.size(); ++index)
	{
		date_interval_bundles[index - 1].date_inter_interval = date_period(
			date_interval_bundles[index - 1].date_interval.end(),
			date_interval_bundles[index].date_interval.begin());
	}*/
}

void DateIntervalBundleStore::ProcessDateGroupsNumber()
{
	std::map<int, int> groups_counter;

	for (auto& date_interval_bundle : date_interval_bundles)
	{
		auto current_group = date_interval_bundle.group;
		
		if (groups_counter.count(current_group))
		{
			groups_counter[current_group] += 1;
		}
		else
		{
			groups_counter[current_group] = 0;
		}

		date_interval_bundle.group_number = groups_counter[current_group];
	}
}

void DateIntervalBundleStore::CheckAndAdjustGroupIntegrity()
{
	for (auto& date_interval_bundle : date_interval_bundles)
	{
		if (date_interval_bundle.group > date_group_store.GetGroupMax())
		{
			date_interval_bundle.group = 0;
		}
	}
}

void DateIntervalBundleStore::AdjustGroupExcludeFlag()
{
	for (auto& bundle : date_interval_bundles)
	{
		bundle.exclude = date_group_store.GetExclude(bundle.group);
	}
}

int DateIntervalBundleStore::GetSpan() const
{
	int span = 0;
	if (date_interval_bundles.size() > 0)
	{
		span = GetLastYear() - GetFirstYear() + 1;
	}
	return span;
}

int DateIntervalBundleStore::GetFirstYear() const
{
	return date_interval_bundles.front().date_interval.begin().year();
}

int DateIntervalBundleStore::GetLastYear() const
{
	return date_interval_bundles.back().date_interval.last().year();
}


void DateIntervalBundleStore::SaveXML(pugi::xml_node* doc)
{
	auto base_node = doc->append_child(L"date_interval_bundles");

	for (size_t index = 0; index < date_interval_bundles.size(); ++index)
	{
		auto node = base_node.append_child(L"date_interval_bundle");

		auto attribute_begin_date = node.append_attribute(L"begin_date");
		std::string begin_date_iso_string = boost::gregorian::to_iso_string(date_interval_bundles[index].date_interval.begin());
		attribute_begin_date.set_value(std::wstring(begin_date_iso_string.begin(), begin_date_iso_string.end()).c_str());

		auto attribute_end_date = node.append_attribute(L"end_date");
		std::string end_date_iso_string = boost::gregorian::to_iso_string(date_interval_bundles[index].date_interval.end());
		attribute_end_date.set_value(std::wstring(end_date_iso_string.begin(), end_date_iso_string.end()).c_str());

		node.append_attribute(L"group").set_value(date_interval_bundles[index].group);

		node.append_attribute(L"comment").set_value(date_interval_bundles[index].comment.c_str());
	}
}


void DateIntervalBundleStore::LoadXML(const pugi::xml_node& doc)
{
	auto base_node = doc.child(L"date_interval_bundles");

	std::vector<DateIntervalBundle> temporary_date_interval_bundles;

	for (auto& node_interval : base_node.children(L"date_interval_bundle"))
	{
		std::wstring begin_date_string = node_interval.attribute(L"begin_date").value();
		std::wstring end_date_string = node_interval.attribute(L"end_date").value();

		date boost_begin_date = boost::gregorian::from_undelimited_string(std::string(begin_date_string.begin(), begin_date_string.end()));
		date boost_end_date = boost::gregorian::from_undelimited_string(std::string(end_date_string.begin(), end_date_string.end()));

		DateIntervalBundle temporary_bundle;

		temporary_bundle.date_interval = date_period(boost_begin_date, boost_end_date);
		temporary_bundle.group = node_interval.attribute(L"group").as_int();
		temporary_bundle.comment = node_interval.attribute(L"comment").value();
		
		//copy constructor, default constructor, constructor??
		temporary_date_interval_bundles.push_back(temporary_bundle);
	}

	SetDateIntervalBundles(temporary_date_interval_bundles);
}





Bar::Bar(const date_period& date_interval) :
	date_interval(date_interval),
	text(L""),
	//admission(0),
	//lenght(0),
	group(0)
{}

void Bar::SetText(const std::wstring text)
{
	this->text = text;
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

std::wstring Bar::GetText() const
{
	return text;
}



void DateIntervalBundleBarStore::SetDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
{
	ProcessDateIntervalBundles(date_interval_bundles);

	ProcessBars();
	ProcessAnnualTotals();
}

void DateIntervalBundleBarStore::ProcessBars()
{
	bars.clear();

	for(size_t index = 0; index < date_interval_bundles.size(); ++index)
	{
		int span = date_interval_bundles[index].date_interval.last().year() - date_interval_bundles[index].date_interval.begin().year();

		std::vector<date_period> splitDatePeriods;
		splitDatePeriods.push_back(date_interval_bundles[index].date_interval);

		for (auto subIndex = 0; subIndex < span; ++subIndex)
		{
			date split_date = date(splitDatePeriods[subIndex].begin().year() + 1, 1, 1);
			splitDatePeriods.push_back(date_period(split_date, splitDatePeriods[subIndex].end()));
			splitDatePeriods[subIndex] = date_period(splitDatePeriods[subIndex].begin(), split_date);
		}

		for (auto subindex = 0U; subindex < splitDatePeriods.size(); ++subindex)
		{
			Bar bar(splitDatePeriods[subindex]);
			if (date_interval_bundles[index].exclude == false)
			{
				bar.SetText(std::to_wstring(date_interval_bundles[index].number + 1));
			}
			if (date_interval_bundles[index].exclude == true)
			{
				std::wstring text_part_0 = std::to_wstring(date_interval_bundles[index].group);
				std::wstring text_part_1 = std::to_wstring(date_interval_bundles[index].group_number);
				
				bar.SetText(std::wstring(L"E") + text_part_0 + std::wstring(L"N") + text_part_1);
			}
			
			bar.group = date_interval_bundles[index].group;
			bars.push_back(bar);
		}	
	}
}

void DateIntervalBundleBarStore::ProcessAnnualTotals()
{
	annualTotals.clear();
	annualTotals.resize(GetSpan());
	
	for (size_t index = 0; index < bars.size(); ++index)
	{
		size_t annualTotalsIndex = static_cast<size_t>(bars[index].GetYear()) - static_cast<size_t>(GetFirstYear());

		annualTotals[annualTotalsIndex] += bars[index].GetLenght();
	}
}

size_t DateIntervalBundleBarStore::GetNumberBars() const
{
	return bars.size();
}

Bar DateIntervalBundleBarStore::GetBar(size_t index) const
{
	return bars[index];
}

int DateIntervalBundleBarStore::GetAnnualTotal(size_t index) const
{
	return annualTotals[index];
}

