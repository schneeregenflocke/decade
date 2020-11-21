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


#pragma once


#include "date_utils.h"
#include "group_store.h"

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>

#include <fstream>
#include <sstream>
#include <array>
#include <map>

#include <pugixml.hpp>



class DateIntervalBundle
{
public:

	DateIntervalBundle() :
		date_interval(boost::gregorian::date_period(boost::gregorian::date(boost::date_time::not_a_date_time), boost::gregorian::date(boost::date_time::not_a_date_time))),
		date_inter_interval(boost::gregorian::date_period(boost::gregorian::date(boost::date_time::not_a_date_time), boost::gregorian::date(boost::date_time::not_a_date_time))),
		number(0),
		group(0),
		group_number(0),
		comment(""),
		exclude(false)
	{};

	boost::gregorian::date_period date_interval;
	boost::gregorian::date_period date_inter_interval;
	int number;
	int group;
	int group_number;
	bool exclude;
	std::string comment;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& date_interval;
		ar& date_inter_interval;
		ar& number;
		ar& group;
		ar& group_number;
		ar& exclude;
		ar& comment;
	}
};


class Bar
{
public:
	Bar(const boost::gregorian::date_period& date_interval) :
		date_interval(date_interval),
		text(""),
		group(0)
	{}

	void SetText(const std::string text)
	{
		this->text = text;
	}

	std::string GetText() const
	{
		return text;
	}

	int GetYear() const
	{
		return date_interval.begin().year();
	}

	int GetLenght() const
	{
		return date_interval.length().days();
	}

	float GetFirstDay() const
	{
		return static_cast<float>(date_interval.begin().day_of_year() - 1);
	}

	float GetLastDay() const
	{
		return static_cast<float>(date_interval.last().day_of_year());
	}

	int group;

private:
	boost::gregorian::date_period date_interval;
	std::string text;
};


class DateIntervalBundleStore
{
public:
	virtual void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
	{
		ProcessDateIntervalBundles(date_interval_bundles);
		signal_date_interval_bundles(this->date_interval_bundles);
	}

	std::vector<DateIntervalBundle> GetDateIntervalBundles() const
	{
		return date_interval_bundles;
	}

	bool is_empty() const
	{
		return date_interval_bundles.empty();
	}

	int GetSpan() const
	{
		int span = 0;
		if (date_interval_bundles.size() > 0)
		{
			span = GetLastYear() - GetFirstYear() + 1;
		}
		return span;
	}

	int GetFirstYear() const
	{
		return date_interval_bundles.front().date_interval.begin().year();
	}

	int GetLastYear() const
	{
		return date_interval_bundles.back().date_interval.last().year();
	}

	void ReceiveDateGroups(const std::vector<DateGroup>& date_groups)
	{
		date_group_store.ReceiveDateGroups(date_groups);
	}

	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_date_interval_bundles;

	/*void LoadXML(const pugi::xml_node& doc)
	{
		auto base_node = doc.child("date_interval_bundles");

		std::vector<DateIntervalBundle> temporary_date_interval_bundles;

		for (auto& node_interval : base_node.children("date_interval_bundle"))
		{
			std::string begin_date_string = node_interval.attribute("begin_date").value();
			std::string end_date_string = node_interval.attribute("end_date").value();

			boost::gregorian::date boost_begin_date = boost::gregorian::from_undelimited_string(std::string(begin_date_string.begin(), begin_date_string.end()));
			boost::gregorian::date boost_end_date = boost::gregorian::from_undelimited_string(std::string(end_date_string.begin(), end_date_string.end()));

			DateIntervalBundle temporary_bundle;

			temporary_bundle.date_interval = boost::gregorian::date_period(boost_begin_date, boost_end_date);
			temporary_bundle.group = node_interval.attribute("group").as_int();
			temporary_bundle.comment = node_interval.attribute("comment").value();

			//copy constructor, default constructor, constructor??
			temporary_date_interval_bundles.push_back(temporary_bundle);
		}

		ReceiveDateIntervalBundles(temporary_date_interval_bundles);
	}*/

	/*void SaveXML(pugi::xml_node* doc)
	{
		auto base_node = doc->append_child("date_interval_bundles");

		for (size_t index = 0; index < date_interval_bundles.size(); ++index)
		{
			auto node = base_node.append_child("date_interval_bundle");

			auto attribute_begin_date = node.append_attribute("begin_date");
			std::string begin_date_iso_string = boost::gregorian::to_iso_string(date_interval_bundles[index].date_interval.begin());
			attribute_begin_date.set_value(std::wstring(begin_date_iso_string.begin(), begin_date_iso_string.end()).c_str());

			auto attribute_end_date = node.append_attribute("end_date");
			std::string end_date_iso_string = boost::gregorian::to_iso_string(date_interval_bundles[index].date_interval.end());
			attribute_end_date.set_value(std::wstring(end_date_iso_string.begin(), end_date_iso_string.end()).c_str());

			node.append_attribute("group").set_value(date_interval_bundles[index].group);

			node.append_attribute("comment").set_value(date_interval_bundles[index].comment.c_str());
		}
	}*/

protected:

	void ProcessDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
	{
		this->date_interval_bundles.clear();
		this->date_interval_bundles.reserve(date_interval_bundles.size());

		// copy, not const reference
		for (auto date_interval_bundle : date_interval_bundles)
		{
			auto case_id = CheckAndAdjustDateInterval(&date_interval_bundle.date_interval);

			if (case_id > 0)
			{
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

	std::vector<DateIntervalBundle> date_interval_bundles;
	
	DateGroupStore date_group_store;

private:

	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive& ar, const unsigned int version) const
	{
		ar& date_interval_bundles;
	}
	template<class Archive>
	void load(Archive& ar, const unsigned int version)
	{
		ar& date_interval_bundles;
		signal_date_interval_bundles(date_interval_bundles);
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	void Sort()
	{
		auto sort_func = [](const DateIntervalBundle& bundle0, const DateIntervalBundle& bundle1) 
		{ 
			return bundle0.date_interval.begin() < bundle1.date_interval.begin(); 
		};

		std::sort(date_interval_bundles.begin(), date_interval_bundles.end(), sort_func);
	}

	void ProcessNumbers()
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

	void ProcessDateInterIntervals()
	{
		auto iterator_first = date_interval_bundles.begin();
		//int counter = 0;

		while (iterator_first != date_interval_bundles.end())
		{
			//std::cout << iterator_first - date_interval_bundles.begin() << '\n';

			bool loop = true;
			while (iterator_first != date_interval_bundles.end() && loop)
			{
				if (iterator_first->exclude == true)
				{
					++iterator_first;
				}
				else
				{
					loop = false;
				}
			}

			if (iterator_first != date_interval_bundles.end())
			{
				auto iterator_second = iterator_first + 1;

				loop = true;
				while (iterator_second != date_interval_bundles.end() && loop)
				{
					if (iterator_second->exclude == true)
					{
						++iterator_second;
					}
					else
					{
						loop = false;
					}
				}

				if (iterator_second != date_interval_bundles.end())
				{
					iterator_first->date_inter_interval = boost::gregorian::date_period(iterator_first->date_interval.end(), iterator_second->date_interval.begin());

					//std::cout << "wrote inter " << counter << '\n';
					//++counter;
				}

				++iterator_first;
			}
			//std::cout << "iterator != end? "  << (iterator_first != date_interval_bundles.end()) << '\n';
		}

		/*for (size_t index = 1; index < date_interval_bundles.size(); ++index)
		{
		date_interval_bundles[index - 1].date_inter_interval = date_period(
		date_interval_bundles[index - 1].date_interval.end(),
		date_interval_bundles[index].date_interval.begin());
		}*/
	}

	void ProcessDateGroupsNumber()
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

	void CheckAndAdjustGroupIntegrity()
	{
		for (auto& date_interval_bundle : date_interval_bundles)
		{
			if (date_interval_bundle.group > date_group_store.GetGroupMax())
			{
				date_interval_bundle.group = 0;
			}
		}
	}

	void AdjustGroupExcludeFlag()
	{
		for (auto& bundle : date_interval_bundles)
		{
			bundle.exclude = date_group_store.GetExclude(bundle.group);
		}
	}
};


class DateIntervalBundleBarStore : public DateIntervalBundleStore
{
public:

	virtual void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles) override
	{
		ProcessDateIntervalBundles(date_interval_bundles);

		ProcessBars();
		ProcessAnnualTotals();
	}

	size_t GetNumberBars() const
	{
		return bars.size();
	}

	Bar GetBar(size_t index) const
	{
		return bars[index];
	}

	int GetAnnualTotal(size_t index) const
	{
		return annualTotals[index];
	}

private:

	void ProcessBars()
	{
		bars.clear();

		for (size_t index = 0; index < date_interval_bundles.size(); ++index)
		{
			int span = date_interval_bundles[index].date_interval.last().year() - date_interval_bundles[index].date_interval.begin().year();

			std::vector<boost::gregorian::date_period> splitDatePeriods;
			splitDatePeriods.push_back(date_interval_bundles[index].date_interval);

			for (auto subIndex = 0; subIndex < span; ++subIndex)
			{
				boost::gregorian::date split_date = boost::gregorian::date(splitDatePeriods[subIndex].begin().year() + 1, 1, 1);
				splitDatePeriods.push_back(boost::gregorian::date_period(split_date, splitDatePeriods[subIndex].end()));
				splitDatePeriods[subIndex] = boost::gregorian::date_period(splitDatePeriods[subIndex].begin(), split_date);
			}

			for (auto subindex = 0U; subindex < splitDatePeriods.size(); ++subindex)
			{
				Bar bar(splitDatePeriods[subindex]);
				if (date_interval_bundles[index].exclude == false)
				{
					bar.SetText(std::to_string(date_interval_bundles[index].number + 1));
				}
				if (date_interval_bundles[index].exclude == true)
				{
					std::string text_part_0 = std::to_string(date_interval_bundles[index].group);
					std::string text_part_1 = std::to_string(date_interval_bundles[index].group_number);

					bar.SetText(std::string("E") + text_part_0 + std::string("N") + text_part_1);
				}

				bar.group = date_interval_bundles[index].group;
				bars.push_back(bar);
			}
		}
	}

	void ProcessAnnualTotals()
	{
		annualTotals.clear();
		annualTotals.resize(GetSpan());

		for (size_t index = 0; index < bars.size(); ++index)
		{
			size_t annualTotalsIndex = static_cast<size_t>(bars[index].GetYear()) - static_cast<size_t>(GetFirstYear());

			annualTotals[annualTotalsIndex] += bars[index].GetLenght();
		}
	}

	std::vector<Bar> bars;
	std::vector<int> annualTotals;
};


class TransformDateIntervalBundle
{
public:

	TransformDateIntervalBundle() : date_shift{ 0, 0 } {};

	void ReceiveDateIntervalBundles(const std::vector<DateIntervalBundle>& date_interval_bundles)
	{
		std::vector<DateIntervalBundle> transformed_bundles = date_interval_bundles;
		auto bundles_iterator = transformed_bundles.begin();

		for (const auto& date_interval_bundle : date_interval_bundles)
		{
			bundles_iterator->date_interval = boost::gregorian::date_period(
				date_interval_bundle.date_interval.begin() + boost::gregorian::date_duration(date_shift[0]),
				date_interval_bundle.date_interval.end() + boost::gregorian::date_duration(date_shift[1]));
			++bundles_iterator;
		}

		signal_transform_date_interval_bundles(transformed_bundles);
	}

	void InputTransformedDateIntervals(const std::vector<DateIntervalBundle>& date_interval_bundles)
	{
		std::vector<DateIntervalBundle> untransformed_bundles = date_interval_bundles;
		auto bundles_iterator = untransformed_bundles.begin();

		for (const auto& date_interval_bundle : date_interval_bundles)
		{
			bundles_iterator->date_interval = boost::gregorian::date_period(
				date_interval_bundle.date_interval.begin() - boost::gregorian::date_duration(date_shift[0]),
				date_interval_bundle.date_interval.end() - boost::gregorian::date_duration(date_shift[1]));
			++bundles_iterator;
		}

		signal_date_interval_bundles(untransformed_bundles);
	}

	void SetTransform(int shift_begin_date, int shift_end_date)
	{
		date_shift[0] = shift_begin_date;
		date_shift[1] = shift_end_date;
	}

	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_date_interval_bundles;
	sigslot::signal<const std::vector<DateIntervalBundle>&> signal_transform_date_interval_bundles;

private:
	std::array<int, 2> date_shift;
};
