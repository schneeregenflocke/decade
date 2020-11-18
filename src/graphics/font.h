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


#ifndef FONT_H
#define FONT_H

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
	void Enumerate();
	std::string GetFilepath(std::string fontname);

private:

	void LoadFreetype();
	void ReleaseFreetype();
	void EnumerateDirectory();

	FT_Library ftlibrary;
	std::map<std::string, std::string> fontname_filepath_mapper;
};


class FontLoader
{
public:
	FontLoader(const std::string& font_path);
	Letter& GetLetterRef(const unsigned char index);

	float TextWidth(const std::string& text, float size);
	float TextHeight(float size);
	float AdjustTextSize(const rect4& cell, std::string text, float height_ratio, float width_ratio);
	
private:
	std::vector<Letter> letters;
};


class FontShape : public Shape<FontShader>
{
public:

	void SetFont(std::shared_ptr<FontLoader> font_loader);

	void SetShapeCentered(const std::string& text, const glm::vec3& position, float size);
	void SetShape(const std::string& text, const glm::vec3& position, float size);

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


#endif /* FONT_H */
