/*
Decade
Copyright (c) 2019-2024 Marco Peyer

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

#include <tabulate/table.hpp>


#include <glad/glad.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <cstddef> // for offsetof


class ShaderInfo
{
public:

	ShaderInfo() :
		info_type(NONE),
		name(""),
		location(0),
		size(0),
		type(0),
		number(0),
		type_size(0),
		type_str("")
	{}

	enum InfoType
	{
		ACTIVE_ATTRIBUTE,
		ACTIVE_UNIFORM,
		NONE
	};

	InfoType info_type;
	std::string name;
	GLint location;
	GLint size;
	GLenum type;
	size_t number;
	size_t type_size;
	std::string type_str;

	void set_type_sizes()
	{
		if (type == GL_FLOAT_VEC2)
		{
			type_str = "GL_FLOAT_VEC2";
			number = 2;
			type_size = sizeof(glm::vec2);
		}
		if (type == GL_FLOAT_VEC3)
		{
			type_str = "GL_FLOAT_VEC3";
			number = 3;
			type_size = sizeof(glm::vec3);
		}
		if (type == GL_FLOAT_VEC4)
		{
			type_str = "GL_FLOAT_VEC4";
			number = 4;
			type_size = sizeof(glm::vec4);
		}
		if (type == GL_FLOAT_MAT4)
		{
			type_str = "GL_FLOAT_MAT4";
			number = 16;
			type_size = sizeof(glm::mat4);
		}
		if (type == GL_SAMPLER_2D)
		{
			type_str = "GL_SAMPLER_2D";
		}
	}

	void Print() const
	{
		std::string info_type_str;

		if (info_type == ACTIVE_ATTRIBUTE)
		{
			info_type_str = "Active Attribute";
		}
		else if (info_type == ACTIVE_UNIFORM)
		{
			info_type_str = "Active Uniform";
		}

		std::cout << "Info_Type: " << info_type_str << ", Name: " << name << ", Location: " << location  << ", Type_String: " << type_str << ", Number: " << number << ", Type_Size: " << type_size << ", Size: " << size << ", Type: " << std::hex << type << std::dec << '\n';
	}
};


class ShaderInfos
{
public:

	ShaderInfos() : 
		program(0)
	{}

	void SetProgram(GLuint program)
	{
		this->program = program;
		
		GatherAttributesInfo();
		GatherUniformsInfo();
	}

	const std::vector<ShaderInfo>& GetAttributesInfos() const
	{
		return attribute_infos;
	}
	
	int GetNumberAttributes() const
	{
		GLint num_attribs;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &num_attribs);
		return num_attribs;
	}

	int GetNumberUniforms() const
	{
		GLint num_uniforms;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);
		return num_uniforms;
	}

	void PrintAttributesInfo() const
	{
		for (const auto& shader_info : attribute_infos)
		{
			shader_info.Print();
		}
	}

	void PrintUniformsInfo() const
	{
		for (const auto& shader_info : uniform_infos)
		{
			shader_info.Print();
		}
	}

private:

	void SortAttributesInfo()
	{
		std::sort(attribute_infos.begin(), attribute_infos.end(), [](const ShaderInfo& a, const ShaderInfo& b) { return a.location < b.location; });
	}

	void SortUniformsInfo()
	{
		std::sort(uniform_infos.begin(), uniform_infos.end(), [](const ShaderInfo& a, const ShaderInfo& b) { return a.location < b.location; });
	}

	void GatherInfo()
	{
	}

	void GatherAttributesInfo()
	{
		GLint num_attribs = GetNumberAttributes();
		GLint max_attrib_name_length = 0;
		glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_attrib_name_length);

		for (int index = 0; index < num_attribs; ++index)
		{
			char* attrib_name_cstr = new char[max_attrib_name_length];
			GLsizei written = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveAttrib(program, index, max_attrib_name_length, &written, &size, &type, attrib_name_cstr);
			GLint location = glGetAttribLocation(program, attrib_name_cstr);

			std::cout << "Written: " << written << ", max_attrib_name_length: " << max_attrib_name_length << '\n';

			ShaderInfo shader_info;
			shader_info.info_type = ShaderInfo::ACTIVE_ATTRIBUTE;
			shader_info.name = std::string(attrib_name_cstr, written);
			shader_info.location = location;
			shader_info.size = size;
			shader_info.type = type;
			shader_info.set_type_sizes();

			delete[] attrib_name_cstr;

			
			attribute_infos.push_back(shader_info);
		}

		SortAttributesInfo();
	}

	void GatherUniformsInfo()
	{
		GLint num_uniforms = GetNumberUniforms();
		GLint max_uniforms_name_length = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_uniforms_name_length);

		for (int index = 0; index < num_uniforms; ++index)
		{
			char* uniform_name_cstr = new char[max_uniforms_name_length];
			GLsizei written = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(program, index, max_uniforms_name_length, &written, &size, &type, uniform_name_cstr);

			GLint location = glGetUniformLocation(program, uniform_name_cstr);

			ShaderInfo shader_info;
			shader_info.info_type = ShaderInfo::ACTIVE_UNIFORM;
			shader_info.name = std::string(uniform_name_cstr, written);
			shader_info.location = location;
			shader_info.size = size;
			shader_info.type = type;
			shader_info.set_type_sizes();

			delete[] uniform_name_cstr;

			uniform_infos.push_back(shader_info);
		}

		SortUniformsInfo();
	}

	GLuint program;
	std::vector<ShaderInfo> attribute_infos;
	std::vector<ShaderInfo> uniform_infos;
};
