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

#include <array>

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/split_member.hpp>


struct PageSetupConfig
{
	std::array<float, 2> size;
	std::array<float, 4> margins;
	int orientation;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& size;
		ar& margins;
		ar& orientation;
	}
};


class PageSetupStore
{
public:

	void ReceivePageSetup(const PageSetupConfig& page_setup_config)
	{
		this->page_setup_config = page_setup_config;
		SendPageSetup();
	}

	void SendPageSetup()
	{
		signal_page_setup_config(page_setup_config);
	}

	sigslot::signal<const PageSetupConfig&> signal_page_setup_config;

	PageSetupConfig page_setup_config;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive& ar, const unsigned int version) const
	{
		ar& page_setup_config;
	}
	template<class Archive>
	void load(Archive& ar, const unsigned int version)
	{
		ar& page_setup_config;
		signal_page_setup_config(page_setup_config);
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
};
