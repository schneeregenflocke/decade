/*
Decade
Copyright (c) 2019-2021 Marco Peyer

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

#include "shaders.h"
#include "shapes_base.h"
#include "texture_object.h"
#include "rect4.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <glad/glad.h>

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <regex>
#include <map>
#include <cstring>
#include <string>
#include <fstream>
#include <memory>


struct Letter
{
	TextureObject texture_object;
	glm::vec2 size;
	glm::vec2 bearing;
	float advance;
};


class EnumerateFont
{
public:

	//EnumerateFont();
	void Enumerate()
	{
		LoadFreetype();
		EnumerateDirectory();
		ReleaseFreetype();
	}
	std::string GetFilepath(std::string fontname)
	{
		std::string filepath = "NotFound";
		if (fontname_filepath_mapper.count(fontname) > 0)
		{
			filepath = fontname_filepath_mapper.at(fontname);
		}
		return filepath;
	}

private:

	void LoadFreetype()
	{
		FT_Error error;

		error = FT_Init_FreeType(&ftlibrary);
		if (error == FT_Err_Ok)
		{
			int major_version;
			int minor_version;
			int patch_version;
			FT_Library_Version(ftlibrary, &major_version, &minor_version, &patch_version);
			std::cout << "FreeType Version " << major_version << "." << minor_version << "." << patch_version << '\n';
		}
		else
		{
			std::cout << "Failed to init FreeType Library" << '\n';
		}
	}
	void ReleaseFreetype()
	{
		FT_Done_FreeType(ftlibrary);
	}
	void EnumerateDirectory()
	{
		std::regex rgx(R"(([\w\-]+).(otf|ttf))", std::regex::icase);

		std::string directory_path = "C:/Windows/Fonts";

		//std::ofstream ofile("font_sfnt_data.txt");

		for (const auto& entry : std::filesystem::directory_iterator(directory_path))
		{
			auto current = entry.path().u8string();

			std::smatch smatch;

			if (std::regex_search(current, smatch, rgx))
			{
				std::string fullpath = directory_path + std::string("/") + smatch[0].str();

				FT_Face ftface;
				FT_Error error = FT_New_Face(ftlibrary, fullpath.c_str(), 0, &ftface);

				if (error == FT_Err_Ok)
				{
					std::string fontname = std::string(ftface->family_name);

					if (std::string(ftface->style_name) != std::string("Regular"))
					{
						fontname += std::string(" ") + std::string(ftface->style_name);
					}

					fontname_filepath_mapper[fontname] = fullpath;
				}
			}
		}
	}

	FT_Library ftlibrary;
	std::map<std::string, std::string> fontname_filepath_mapper;
};


class FontLoader
{
public:
	FontLoader(const std::string& font_path)
	{
		FT_Library ftlibrary;
		FT_Error error;

		error = FT_Init_FreeType(&ftlibrary);
		if (error == FT_Err_Ok)
		{
			int major_version;
			int minor_version;
			int patch_version;
			FT_Library_Version(ftlibrary, &major_version, &minor_version, &patch_version);
			std::cout << "FreeType Version " << major_version << "." << minor_version << "." << patch_version << '\n';
		}
		else
		{
			std::cout << "Failed to init FreeType Library" << '\n';
		}

		FT_Face ftface;
		error = FT_New_Face(ftlibrary, font_path.c_str(), 0, &ftface);
		if (error)
		{
			std::cout << "Failed to load font" << '\n';
		}
		else
		{
			std::cout << "Font " << font_path << " " << ftface->family_name << " loaded" << '\n';
		}

		const FT_UInt fontPixelHeight = 128;
		FT_Set_Pixel_Sizes(ftface, 0, fontPixelHeight);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glActiveTexture(GL_TEXTURE0);

		const size_t number_letters = 256;
		letters.resize(number_letters);

		for (size_t index = 0; index < number_letters; ++index)
		{
			error = FT_Load_Char(ftface, index, FT_LOAD_RENDER);
			if (error)
			{
				std::cout << "Failed to load Glyph " << index << '\n';
				continue;
			}

			GLuint texture;
			texture = letters[index].texture_object.Texture();
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ftface->glyph->bitmap.width, ftface->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, ftface->glyph->bitmap.buffer);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glBindTexture(GL_TEXTURE_2D, 0);

			//////////////////////////////////

			float float_font_height = static_cast<float>(fontPixelHeight);

			float sizex = static_cast<float>(ftface->glyph->bitmap.width) / float_font_height;
			float sizey = static_cast<float>(ftface->glyph->bitmap.rows) / float_font_height;
			letters[index].size = glm::vec2(sizex, sizey);

			float bearingx = static_cast<float>(ftface->glyph->bitmap_left) / float_font_height;
			float bearingy = static_cast<float>(ftface->glyph->bitmap_top) / float_font_height;
			letters[index].bearing = glm::vec2(bearingx, bearingy);

			letters[index].advance = static_cast<float>(ftface->glyph->advance.x) / 64.f / float_font_height;
		}

		FT_Done_Face(ftface);
		FT_Done_FreeType(ftlibrary);
	}
	Letter& GetLetterRef(const unsigned char index)
	{
		return letters[index];
	}

	float TextWidth(const std::string& text, float size)
	{
		float width = 0.f;
		for (char index = 0; index < text.size(); ++index)
		{
			const char letter_number = text[index];
			width += GetLetterRef(letter_number).advance * size;
		}

		return width;
	}
	float TextHeight(float size)
	{
		std::vector<size_t> index_list;

		for (size_t index = 48; index <= 57; ++index)
		{
			index_list.push_back(index);
		}
		for (size_t index = 65; index <= 90; ++index)
		{
			index_list.push_back(index);
		}
		for (size_t index = 97; index <= 122; ++index)
		{
			index_list.push_back(index);
		}

		float height = 0.f;
		for (size_t index = 0; index < index_list.size(); ++index)
		{
			float currentCharacterBearing = GetLetterRef(index).bearing.y * size;

			if (currentCharacterBearing > height)
			{
				height = currentCharacterBearing;
			}
		}

		return height;
	}
	float AdjustTextSize(const rect4& cell, std::string text, float height_ratio, float width_ratio)
	{
		float font_size = cell.Height() * height_ratio;

		auto text_width = TextWidth(text, font_size);

		auto ratio = font_size / text_width;

		if (text_width > cell.Width() * width_ratio)
		{
			font_size = ratio * cell.Width() * width_ratio;
		}

		return font_size;
	}
	
private:
	std::vector<Letter> letters;
};


class FontShape : public Shape<FontShader>
{
public:

	void SetFont(std::shared_ptr<FontLoader> font_loader)
	{
		this->font_loader = font_loader;
	}

	void SetShapeCentered(const std::string& text, const glm::vec3& position, float size)
	{
		if (font_loader != nullptr)
		{
			auto half_wdith = font_loader->TextWidth(text, size) / 2.f;
			auto half_height = font_loader->TextHeight(size) / 2.f;

			SetShape(text, position - vec3(half_wdith, half_height, 0.f), size);
		}
	}
	void SetShape(const std::string& text, const glm::vec3& position, float size)
	{
		if (font_loader != nullptr)
		{
			SetBufferSize(text.size() * 6);

			float currentx = position.x;
			float currenty = position.y;

			textTextures.resize(text.size());

			for (size_t index = 0; index < text.size(); ++index)
			{
				size_t letter_index = text[index];

				GLuint texture = font_loader->GetLetterRef(letter_index).texture_object.Texture();
				textTextures[index] = texture;

				GLfloat xpos = currentx + font_loader->GetLetterRef(letter_index).bearing.x * size;
				GLfloat ypos = currenty - (font_loader->GetLetterRef(letter_index).size.y - font_loader->GetLetterRef(letter_index).bearing.y) * size;

				GLfloat width = font_loader->GetLetterRef(letter_index).size.x * size;
				GLfloat height = font_loader->GetLetterRef(letter_index).size.y * size;

				GetVertexRef(index * 6 + 0).position = glm::vec3(xpos, ypos + height, 0.f);
				GetVertexRef(index * 6 + 1).position = glm::vec3(xpos, ypos, 0.f);
				GetVertexRef(index * 6 + 2).position = glm::vec3(xpos + width, ypos, 0.f);
				GetVertexRef(index * 6 + 3).position = glm::vec3(xpos, ypos + height, 0.f);
				GetVertexRef(index * 6 + 4).position = glm::vec3(xpos + width, ypos, 0.f);
				GetVertexRef(index * 6 + 5).position = glm::vec3(xpos + width, ypos + height, 0.f);

				GetVertexRef(index * 6 + 0).texturePosition = glm::vec2(0.0f, 0.0f);
				GetVertexRef(index * 6 + 1).texturePosition = glm::vec2(0.0f, 1.0f);
				GetVertexRef(index * 6 + 2).texturePosition = glm::vec2(1.0f, 1.0f);
				GetVertexRef(index * 6 + 3).texturePosition = glm::vec2(0.0f, 0.0f);
				GetVertexRef(index * 6 + 4).texturePosition = glm::vec2(1.0f, 1.0f);
				GetVertexRef(index * 6 + 5).texturePosition = glm::vec2(1.0f, 0.0f);

				currentx += font_loader->GetLetterRef(letter_index).advance * size;
			}

			UpdateBuffer();

		}
	}

	void Draw() const override
	{
		shader->UseProgram();
		
		auto colorLocation = glGetUniformLocation(shader->GetProgram(), "texColor");
		glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
		glUniform4f(colorLocation, color.r, color.g, color.b, color.a);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glActiveTexture(GL_TEXTURE0);

		for (size_t index = 0; index < textTextures.size(); ++index)
		{
			glBindTexture(GL_TEXTURE_2D, textTextures[index]);
			glDrawArrays(GL_TRIANGLES, static_cast<GLint>(index) * 6, 6);
		}
	}

private:

	std::vector<GLuint> textTextures;
	std::shared_ptr<FontLoader> font_loader;
};
