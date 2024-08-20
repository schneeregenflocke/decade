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

#include "shaders_info.hpp"

#include <glad/glad.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <optional>

#include "Resource.h"


class Shader
{
public:

	Shader(const std::string& vertex_shader_source, const std::string& fragment_shader_source, const std::string& name) :
		program(0),
		name(name)
	{
		CompileProgram(vertex_shader_source, fragment_shader_source);

		shader_info.SetProgram(program);
	}

	GLuint GetProgram() const
	{
		return program;
	}

	std::string get_name() const
	{
		return name;
	}

	void UseProgram() const
	{
		glUseProgram(program);
	}

	void SetUniform(std::string name, const glm::mat4& matrix) const
	{
		auto location = glGetUniformLocation(program, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void SetUniform(std::string name, const glm::vec4& vector) const
	{
		auto location = glGetUniformLocation(program, name.c_str());
		glUniform4fv(location, 1, glm::value_ptr(vector));
		//glProgramUniform
	}

	void PrintShaderInfo() const
	{
		std::cout << "Shader: " << name << ", Number of attributes: " << shader_info.GetNumberAttributes() << ", Number of uniforms: " << shader_info.GetNumberUniforms() << '\n';
		
		shader_info.PrintAttributesInfo();
		shader_info.PrintUniformsInfo();
	}

	const ShaderInfos& GetShaderInfo() const
	{
		return shader_info;
	}

	const std::vector<ShaderInfo>& GetShaderAttributesInfos() const
	{
		return shader_info.GetAttributesInfos();
	}

private:

	void CompileProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source)
	{
		GLuint vertex_shader = CompileShader(vertex_shader_source, GL_VERTEX_SHADER);
		GLuint fragment_shader = CompileShader(fragment_shader_source, GL_FRAGMENT_SHADER);
		
		LinkShaders(vertex_shader, fragment_shader);
	}

	void LinkShaders(const GLuint vertex_shader, const GLuint fragment_shader)
	{
		program = glCreateProgram();

		glAttachShader(program, vertex_shader);
		glAttachShader(program, fragment_shader);

		glLinkProgram(program);

		glValidateProgram(program);

		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		std::cout << "GL_LINK_STATUS: " << program << " " << std::boolalpha << static_cast<bool>(status) << std::noboolalpha << '\n';

		GLint info_lenght;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_lenght);
		if (info_lenght > 0)
		{
			char* info_log = new char[info_lenght];
			glGetProgramInfoLog(program, info_lenght, NULL, info_log);
			std::cout << info_log;
			delete[] info_log;
		}

		glDetachShader(program, vertex_shader);
		glDetachShader(program, fragment_shader);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	GLuint CompileShader(const std::string& source, const GLuint shadertype)
	{
		GLuint shader = glCreateShader(shadertype);
		const char* source_cstr = source.c_str();
        glShaderSource(shader, 1, &source_cstr, nullptr);
		glCompileShader(shader);

		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		std::cout << "GL_COMPILE_STATUS: " << shader << " " << std::boolalpha << static_cast<bool>(status) << std::noboolalpha << '\n';

		GLint info_lenght;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_lenght);
		if (info_lenght > 0)
		{
			char* info_log = new char[info_lenght];
			glGetShaderInfoLog(shader, info_lenght, NULL, info_log);
			std::cout << info_log;
			delete[] info_log;
		}
		
		return shader;
	}

	GLuint program;
	std::string name;
	ShaderInfos shader_info;
};


class Shaders
{
public:
	Shaders()
	{
		auto simple_vertex_shader_resource = LOAD_RESOURCE(shader_simple_vertex_shader);
		auto simple_fragment_shader_resource = LOAD_RESOURCE(shader_simple_fragment_shader);
		auto rectangles_vertex_shader_resource = LOAD_RESOURCE(shader_rectangles_vertex_shader);
		auto rectangles_fragment_shader_resource = LOAD_RESOURCE(shader_rectangles_fragment_shader);
		auto font_vertex_shader_resource = LOAD_RESOURCE(shader_font_vertex_shader);
		auto font_fragment_shader_resource = LOAD_RESOURCE(shader_font_fragment_shader);
		//auto phong_vertex_shader_resource = LOAD_RESOURCE(shader_phong_vertex_shader);
		//auto phong_fragment_shader_resource = LOAD_RESOURCE(shader_phong_fragment_shader);

		shaders.push_back(Shader(simple_vertex_shader_resource.toString(), simple_fragment_shader_resource.toString(), std::string("Simple Shader")));
		shaders.push_back(Shader(rectangles_vertex_shader_resource.toString(), rectangles_fragment_shader_resource.toString(), std::string("Rectangles Shader")));
		shaders.push_back(Shader(font_vertex_shader_resource.toString(), font_fragment_shader_resource.toString(), std::string("Font Shader")));
		//shaders.push_back(Shader(phong_vertex_shader_resource.toString(), phong_fragment_shader_resource.toString(), std::string("Phong Shader")));

		PrintInfo();
	}

	void PrintInfo() const
	{
		for (size_t index = 0; index < shaders.size(); ++index)
		{
			shaders[index].PrintShaderInfo();
		}
	}

	Shader* GetShader(size_t index)
	{
		if (index < shaders.size())
		{
			return &shaders[index];
		}
		else
		{
			throw std::invalid_argument("Shader index out of bounds");
		}
	}

	std::optional<Shader*> search_shader(const std::string& search_name)
	{
		for (auto& shader : shaders)
		{
			if (shader.get_name() == search_name)
			{
				return std::optional<Shader*>(&shader);
			}
		}
		return std::nullopt;
	}

	size_t GetNumberShaders() const
	{
		return shaders.size();
	}

private:
	std::vector<Shader> shaders;
};

