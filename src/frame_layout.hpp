/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


#pragma once


#include "graphics/rect.hpp"
#include <vector>
#include <numeric>


class ProportionFrameLayout
{
public:

	void SetupRowFrames(const rectf& frame, const size_t number_rows)
	{
		row_frames.resize(number_rows);

		auto row_height = frame.height() / static_cast<float>(number_rows);

		for (size_t index = 0; index < row_frames.size(); ++index)
		{
			auto float_index = static_cast<float>(index);
			auto current_bottom = frame.b() + float_index * row_height;
			row_frames[index] = rectf(frame.l(), frame.r(), current_bottom, current_bottom + row_height);
		}
	}

	void SetupSubFrames(const std::vector<float>& proportions)
	{
		number_sub_frames_per_row = (proportions.size() - 1) / 2;

		sub_frames.resize(row_frames.size() * number_sub_frames_per_row);

		for (size_t index = 0; index < row_frames.size(); ++index)
		{
			std::vector<float> sections = Section(proportions, row_frames[index].height());
			std::vector<float> cumulative_sections(sections.size());

			for (size_t subindex = 0; subindex < cumulative_sections.size(); ++subindex)
			{
				cumulative_sections[subindex] = std::accumulate(sections.cbegin(), sections.cbegin() + subindex, 0.f);
			}

			for (size_t subindex = 0; subindex < number_sub_frames_per_row; ++subindex)
			{
				sub_frames[index * number_sub_frames_per_row + subindex].setL(row_frames[index].l());
				sub_frames[index * number_sub_frames_per_row + subindex].setR(row_frames[index].r());
				sub_frames[index * number_sub_frames_per_row + subindex].setB(row_frames[index].b() + cumulative_sections[subindex * 2 + 1]);
				sub_frames[index * number_sub_frames_per_row + subindex].setT(row_frames[index].b() + cumulative_sections[subindex * 2 + 2]);
			}
		}
	}

	rectf GetSubFrame(const size_t row, const size_t sub) const
	{
		return sub_frames[number_sub_frames_per_row * row + sub];
	}

private:

	std::vector<float> Section(const std::vector<float>& proportions, float value) const
	{
		auto sum = std::accumulate(proportions.cbegin(), proportions.cend(), 0.f);

		std::vector<float> sections;
		sections.reserve(proportions.size());

		for (const auto& proportion : proportions)
		{
			sections.push_back((proportion / sum) * value);
		}

		return sections;
	}

	std::vector<rectf> row_frames;
	std::vector<rectf> sub_frames;
	size_t number_sub_frames_per_row;
};
