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


#include "shaders.hpp"
#include "shapes_base.hpp"
#include "texture_object.hpp"
#include "rect.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_IMAGE_H

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <glad/glad.h>

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <exception>


struct Letter
{
	Texture texture_object;
	glm::vec2 size;
	glm::vec2 bearing;
	float advance;
};


class Font
{
public:
	
	/*Font(const std::vector<unsigned char>& font_data) :
		ft_library(nullptr), ft_face(nullptr), letters(256)
	{
		InitFreetype();
		LoadFont(font_data);
		LoadTextures();
		ReleaseFreetype();
	}*/

	Font(const std::string& filepath) :
		ft_library(nullptr), ft_face(nullptr), letters(256)
	{
		InitFreetype();
		LoadFont(filepath);
		LoadTextures();
		ReleaseFreetype();
	}

	const Letter& GetLetterRef(const unsigned char index) const
	{
		return letters[index];
	}

	float TextWidth(const std::string& text, float size) const
	{
		float width = 0.f;
		for (char index = 0; index < text.size(); ++index)
		{
			const char letter_number = text[index];
			width += GetLetterRef(letter_number).advance * size;
		}

		return width;
	}

	float TextHeight(float size) const
	{
		std::vector<size_t> index_list;
		std::array<std::array<size_t, 2>, 3> char_intervals;

		char_intervals[0] = { 48, 57 };
		char_intervals[1] = { 65, 90 };
		char_intervals[2] = { 97, 122 };

		for (const auto& char_interval : char_intervals)
		{
			for (size_t index = char_interval[0]; index <= char_interval[1]; ++index)
			{
				index_list.push_back(index);
			}
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

	float AdjustTextSize(const rectf& cell, std::string text, float height_ratio, float width_ratio) const
	{
		float font_size = cell.height() * height_ratio;
		auto text_width = TextWidth(text, font_size);
		auto ratio = font_size / text_width;

		if (text_width > cell.width() * width_ratio)
		{
			font_size = ratio * cell.width() * width_ratio;
		}

		return font_size;
	}

private:

	void InitFreetype()
	{
		FT_Error ft_error = FT_Init_FreeType(&ft_library);
		if (ft_error == FT_Err_Ok)
		{
			PrintVersion();
		}
		else
		{
			throw std::runtime_error(std::string("Freetype FT_Init_FreeType failed ") + std::to_string(ft_error));
		}
	}

	void ReleaseFreetype()
	{
		FT_Error ft_error = FT_Done_Face(ft_face);
		if (ft_error != FT_Err_Ok)
		{
			throw std::runtime_error(std::string("Freetype FT_Done_Face failed ") + std::to_string(ft_error));
		}
		else
		{
			ft_face = nullptr;
		}
		ft_error = FT_Done_FreeType(ft_library);
		if (ft_error != FT_Err_Ok)
		{
			throw std::runtime_error(std::string("Freetype FT_Done_FreeType failed ") + std::to_string(ft_error));
		}
		else
		{
			ft_library = nullptr;
		}
	}

	void PrintVersion()
	{
		std::array<FT_Int, 3> version;
		FT_Library_Version(ft_library, &version[0], &version[1], &version[2]);
		std::cout << "FreeType Version " << version[0] << "." << version[1] << "." << version[2] << '\n';
	}

	void LoadFont(const std::string& file_path)
	{
		FT_Error ft_error = FT_New_Face(ft_library, file_path.c_str(), 0, &ft_face);
		if (ft_error == FT_Err_Ok)
		{
			std::cout << "ft_face->family_name " << ft_face->family_name <<  '\n';
		}
		else
		{
			throw std::runtime_error(std::string("Freetype FT_New_Memory_Face failed ") + std::to_string(ft_error));
		}
	}

	/*void LoadFont(const std::vector<unsigned char>& font_data)
	{
		FT_Error ft_error = FT_New_Memory_Face(ft_library, font_data.data(), font_data.size(), 0, &ft_face);
		if (ft_error == FT_Err_Ok)
		{
			std::cout << "ft_face->family_name " << ft_face->family_name <<  '\n';
		}
		else
		{
			throw std::runtime_error(std::string("Freetype FT_New_Memory_Face failed ") + std::to_string(ft_error));
		}
	}*/
		
	void LoadTextures()
	{
		const FT_UInt font_pixel_height = 2048;
		//const FT_UInt font_pixel_height = 128;
		FT_Error ft_error = FT_Set_Pixel_Sizes(ft_face, 0, font_pixel_height);

		//FT_CONFIG_OPTION_ERROR_STRINGS, FT_DEBUG_LEVEL_ERROR
		auto ft_error_string = FT_Error_String(ft_error);
		int ft_error_string_len = strlen(ft_error_string);
		std::string error_string = std::string(ft_error_string, ft_error_string + ft_error_string_len);
		if (ft_error != FT_Err_Ok)
		{
			std::cout << "FreeType Error: " << error_string << '\n';
			throw std::runtime_error(std::string("Freetype FT_Set_Pixel_Sizes failed ") + std::to_string(ft_error));
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glActiveTexture(GL_TEXTURE0);

		//const size_t number_letters = 256;
		//letters.resize(number_letters);
		for (size_t index = 0; index < letters.size(); ++index)
		{
			//FT_Error ft_error = FT_Load_Char(ft_face, index, FT_LOAD_RENDER);
			FT_Error ft_error = FT_Load_Char(ft_face, index, FT_LOAD_DEFAULT);
			if (ft_error != FT_Err_Ok)
			{
				throw std::runtime_error(std::string("Freetype FT_Load_Char failed ") + std::to_string(ft_error));
			}

			FT_Glyph glyph;
			ft_error = FT_Get_Glyph(ft_face->glyph, &glyph);
			if (ft_error != FT_Err_Ok)
			{
				throw std::runtime_error(std::string("Freetype FT_Get_Glyph failed ") + std::to_string(ft_error));
			}
			
			FT_Glyph glyph_bitmap = glyph;
			ft_error = FT_Glyph_To_Bitmap(&glyph_bitmap, FT_RENDER_MODE_NORMAL, 0, true);
			if (ft_error != FT_Err_Ok)
			{
				throw std::runtime_error(std::string("Freetype FT_Glyph_To_Bitmap failed ") + std::to_string(ft_error));
			}
			if (glyph_bitmap->format != FT_GLYPH_FORMAT_BITMAP)
			{
				throw std::runtime_error(std::string("Freetype glyph->format != FT_GLYPH_FORMAT_BITMAP"));
			}

			FT_BitmapGlyph glyph_bitmap_cast = (FT_BitmapGlyph)glyph_bitmap;
			
			const auto bitmap_width = glyph_bitmap_cast->bitmap.width;
			const auto bitmap_height = glyph_bitmap_cast->bitmap.rows;

			GLuint texture = letters[index].texture_object.Name();
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap_width, bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, glyph_bitmap_cast->bitmap.buffer);
			//glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);

			float float_font_height = static_cast<float>(font_pixel_height);

			float sizex = static_cast<float>(bitmap_width) / float_font_height;
			float sizey = static_cast<float>(bitmap_height) / float_font_height;
			letters[index].size = glm::vec2(sizex, sizey);

			float bearingx = static_cast<float>(ft_face->glyph->bitmap_left) / float_font_height;
			float bearingy = static_cast<float>(ft_face->glyph->bitmap_top) / float_font_height;
			letters[index].bearing = glm::vec2(bearingx, bearingy);
			letters[index].advance = static_cast<float>(ft_face->glyph->advance.x) / 64.f / float_font_height;

			FT_Done_Glyph(glyph_bitmap);
		}
	}

	FT_Library ft_library;
	FT_Face ft_face;
	std::vector<Letter> letters;
};


class FontShape : public Shape<FontShader>
{
public:

	explicit FontShape(const std::string& shape_name, const std::shared_ptr<Font>& font) :
		Shape<FontShader>(shape_name),
		font(font)
	{}

	void SetShape(const std::string& text, const glm::vec3& position, float size)
	{
		SetBufferSize(text.size() * 6);

		text_textures.resize(text.size());

		float current_x = position.x;
		float current_y = position.y;

		for (size_t index = 0; index < text.size(); ++index)
		{
			const size_t letter_index = text[index];

			GLuint texture = font->GetLetterRef(letter_index).texture_object.Name();
			text_textures[index] = texture;

			GLfloat xpos = current_x + font->GetLetterRef(letter_index).bearing.x * size;
			GLfloat ypos = current_y - (font->GetLetterRef(letter_index).size.y - font->GetLetterRef(letter_index).bearing.y) * size;

			GLfloat width = font->GetLetterRef(letter_index).size.x * size;
			GLfloat height = font->GetLetterRef(letter_index).size.y * size;

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

			current_x += font->GetLetterRef(letter_index).advance * size;
		}

		UpdateBuffer();	
	}

	void SetShapeCentered(const std::string& text, const glm::vec3& position, float size)
	{
		auto half_wdith = font->TextWidth(text, size) / 2.f;
		auto half_height = font->TextHeight(size) / 2.f;

		SetShape(text, position - glm::vec3(half_wdith, half_height, 0.f), size);
	}

	void Draw() const override
	{
		shader_ptr->UseProgram();
		
		auto colorLocation = glGetUniformLocation(shader_ptr->GetProgram(), "texColor");
		glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
		glUniform4f(colorLocation, color.r, color.g, color.b, color.a);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glActiveTexture(GL_TEXTURE0);

		for (size_t index = 0; index < text_textures.size(); ++index)
		{
			glBindTexture(GL_TEXTURE_2D, text_textures[index]);
			glDrawArrays(GL_TRIANGLES, static_cast<GLint>(index) * 6, 6);
			
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}

private:
	std::vector<GLuint> text_textures;
	std::shared_ptr<Font> font;
};
