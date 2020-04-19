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

#include "font.h"


void EnumerateFont::Enumerate()
{
	LoadFreetype();
	EnumerateDirectory();
	ReleaseFreetype();
}

std::string EnumerateFont::GetFilepath(std::string fontname)
{
	std::string filepath = "NotFound";
	if (fontname_filepath_mapper.count(fontname) > 0)
	{
		filepath = fontname_filepath_mapper.at(fontname);
	}
	return filepath;
}

void EnumerateFont::LoadFreetype()
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

void EnumerateFont::ReleaseFreetype()
{
	FT_Done_FreeType(ftlibrary);
}

void EnumerateFont::EnumerateDirectory()
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


FontLoader::FontLoader(const std::string& font_path)
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

Letter& FontLoader::GetLetterRef(size_t index)
{
	return letters[index];
}




FontShape::FontShape()
{
	font = nullptr;
}

void FontShape::SetFont(FontLoader* value)
{
	font = value;
}

/*float FontShape::CalcSizeAdjustment(float size)
{
	return (1.f / TextHeight(1.f)) * size;
}*/

float FontShape::TextWidth(const std::wstring& text, float size)
{
	//float size_adjustment = CalcSizeAdjustment(size);

	float width = 0.f;

	for (size_t index = 0; index < text.size(); ++index)
	{
		width += font->GetLetterRef(text[index]).advance * size;
	}

	return width;
}

float FontShape::TextHeight(float size)
{
	float height = 0.f;

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

	for (size_t index = 0; index < index_list.size(); ++index)
	{
		float currentCharacterBearing = font->GetLetterRef(index).bearing.y * size;

		if (currentCharacterBearing > height)
		{
			height = currentCharacterBearing;
		}
	}

	return height;
}


void FontShape::SetShapeHVCentered(const std::wstring& text, const glm::vec3& position, float size)
{
	if (font != nullptr)
	{
		auto half_wdith = TextWidth(text, size) / 2.f;
		auto half_height = TextHeight(size) / 2.f;
		//auto half_height = size / 2.f;
		//auto half_height = 0;

		SetShape(text, position - vec3(half_wdith, half_height, 0.f), size);
	}
}

void FontShape::SetShape(const std::wstring& text, const glm::vec3& position, float size)
{
	//float size_adjustment = CalcSizeAdjustment(size);

	if (font != nullptr)
	{
		SetBufferSize(text.size() * 6);

		float currentx = position.x;
		float currenty = position.y;

		textTextures.resize(text.size());

		for (size_t index = 0; index < text.size(); ++index)
		{
			size_t letter_index = text[index];

			GLuint texture = font->GetLetterRef(letter_index).texture_object.Texture();
			textTextures[index] = texture;

			GLfloat xpos = currentx + font->GetLetterRef(letter_index).bearing.x * size;
			GLfloat ypos = currenty - (font->GetLetterRef(letter_index).size.y - font->GetLetterRef(letter_index).bearing.y) * size;

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

			currentx += font->GetLetterRef(letter_index).advance * size;
		}

		UpdateBuffer();

	}
	
	
}


