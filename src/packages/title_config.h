/*
Decade
Copyright (c) 2019-2020 Marco Peyer

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


#include <array>
#include <string>

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/string.hpp>


struct TitleConfig
{
	TitleConfig() :
		frame_height(10.f),
		font_size_ratio(1.f),
		title_text("title config constructor text"),
		text_color{ 0.f, 0.f, 0.f, 1.f }
	{}

	float frame_height;
	float font_size_ratio;
	std::string title_text;
	std::array<float, 4> text_color;

private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& BOOST_SERIALIZATION_NVP(frame_height);
		ar& BOOST_SERIALIZATION_NVP(font_size_ratio);
		ar& BOOST_SERIALIZATION_NVP(title_text);
		ar& BOOST_SERIALIZATION_NVP(text_color);
	}
};


class TitleConfigStore
{
public:

	void SendTitleConfig()
	{
		signal_title_config(title_config);
	}

	void ReceiveTitleConfig(const TitleConfig& title_config)
	{
		this->title_config = title_config;
		SendTitleConfig();
	}

	sigslot::signal<const TitleConfig&> signal_title_config;


private:

	TitleConfig title_config;

	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive& ar, const unsigned int version) const
	{
		ar& BOOST_SERIALIZATION_NVP(title_config);
	}
	template<class Archive>
	void load(Archive& ar, const unsigned int version)
	{
		ar& BOOST_SERIALIZATION_NVP(title_config);
		signal_title_config(title_config);
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
};
