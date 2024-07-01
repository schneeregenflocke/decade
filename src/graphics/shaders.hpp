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


#include <glad/glad.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstddef> // for offsetof

#include "Resource.h"



struct VertexVC
{
	glm::vec3 point;
	glm::vec4 color;

	static void SetAttribPointers()
	{
		GLsizei stride = sizeof(VertexVC);

		auto offsetof0 = offsetof(VertexVC, point);
		auto offsetof1 = offsetof(VertexVC, color);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof0));
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof1));
	}
	static void EnableVertexAttribArrays()
	{
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
	static void DisableVertexAttribArrays()
	{
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}
};


struct VertexVT
{
	glm::vec3 position;
	glm::vec2 texturePosition;

	static void SetAttribPointers()
	{
		GLsizei stride = sizeof(VertexVT);

		//auto offsetof0 = offsetof(VertexVT, position);
		//auto offsetof1 = offsetof(VertexVT, texturePosition);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(VertexVT, position)));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(VertexVT, texturePosition)));
	}
	static void EnableVertexAttribArrays()
	{
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
	static void DisableVertexAttribArrays()
	{
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}
};




class Shader
{
public:

	Shader() : 
		program(0),
		name("")
	{
	}

	Shader(const std::string& vertex_shader_source, const std::string& fragment_shader_source, const std::string& name) :
		program(0),
		name(name)
	{
		CompileProgram(vertex_shader_source, fragment_shader_source);

		GatherAttributesInfo();
		GatherUniformsInfo();
	}

	GLuint GetProgram() const
	{
		return program;
	}

	std::string GetName() const
	{
		return name;
	}

	void UseProgram() const
	{
		glUseProgram(program);
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

	void PrintShaderInfo() const
	{
		std::cout << "Shader: " << name << '\n';
		std::cout << "Number of attributes: " << GetNumberAttributes() << '\n';
		std::cout << "Number of uniforms: " << GetNumberUniforms() << '\n';

		for(const auto& shader_info : shader_infos)
		{
			shader_info.Print();
		}
	}

	/*int GetUniformLocation(const std::string& name) const
	{
		return glGetUniformLocation(program, name.c_str());
	}*/

	/*void SetUniform(GLint location, const glm::mat4& matrix) const
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}*/

	void SetUniform(std::string name, const glm::mat4& matrix) const
	{
		auto location = glGetUniformLocation(program, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void SetUniform(std::string name, const glm::vec4& vector) const
	{
		auto location = glGetUniformLocation(program, name.c_str());
		glUniform4fv(location, 1, glm::value_ptr(vector));
	}

protected:

	void CompileProgram(Resource vertexshaderfile, Resource fragmentshaderfile)
	{
		GLuint vertexshader = CompileShader(vertexshaderfile, GL_VERTEX_SHADER);
		GLuint fragmentshader = CompileShader(fragmentshaderfile, GL_FRAGMENT_SHADER);
		program = LinkShaders(vertexshader, fragmentshader);
	}

	void CompileProgram(const std::string& vertex_shader_source, const std::string& fragment_shader_source)
	{
		GLuint vertex_shader = CompileShader(vertex_shader_source, GL_VERTEX_SHADER);
		GLuint fragment_shader = CompileShader(fragment_shader_source, GL_FRAGMENT_SHADER);
		program = LinkShaders(vertex_shader, fragment_shader);
	}

	static GLuint LinkShaders(const GLuint vertex_shader, const GLuint fragment_shader)
	{
		GLuint program = glCreateProgram();

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

		return program;
	}

	static GLuint CompileShader(const std::string& source, const GLuint shadertype)
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

	static GLuint CompileShader(const Resource& resource, const GLuint shadertype)
	{
		return CompileShader(resource.toString(), shadertype);
	}

private:

	struct ShaderInfo
	{
		enum InfoType
		{
			ACTIVE_ATTRIBUTES,
			ACTIVE_UNIFORMS
		};

		InfoType info_type;
		std::string name;
		GLint location;
		GLint size;
		GLenum type;

		void Print() const
		{
			std::string info_type_str;
			if (info_type == ACTIVE_ATTRIBUTES)
			{
				info_type_str = "Active Attributes";
			}
			else if (info_type == ACTIVE_UNIFORMS)
			{
				info_type_str = "Active Uniforms";
			}

			std::cout << "Info_Type: " << info_type_str << ", Name: " << name << ", Location: " << location << ", Size: " << size << ", Type: " << std::hex << type << std::dec <<'\n';
		}
	};

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
			shader_info.info_type = ShaderInfo::ACTIVE_ATTRIBUTES;
			shader_info.name = std::string(attrib_name_cstr, written);
			shader_info.location = location;
			shader_info.size = size;
			shader_info.type = type;

			delete[] attrib_name_cstr;

			shader_infos.push_back(shader_info);
		}
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
			shader_info.info_type = ShaderInfo::ACTIVE_UNIFORMS;
			shader_info.name = std::string(uniform_name_cstr, written);
			shader_info.location = location;
			shader_info.size = size;
			shader_info.type = type;

			delete[] uniform_name_cstr;

			shader_infos.push_back(shader_info);
		}
	}

	GLuint program;
	std::string name;
	std::vector<ShaderInfo> shader_infos;
};


class ProjectionShader
{
public:

	void GetProjectionLocations(GLuint program)
	{
		model = glGetUniformLocation(program, "model");
		view = glGetUniformLocation(program, "view");
		projection = glGetUniformLocation(program, "projection");
	}

	GLint model;
	GLint view;
	GLint projection;

	void SetModelMatrix(const glm::mat4& matrix) const
	{
		glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void SetViewMatrix(const glm::mat4& matrix) const
	{
		glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void SetProjectionMatrix(const glm::mat4& matrix) const
	{
		glUniformMatrix4fv(projection, 1, GL_FALSE, glm::value_ptr(matrix));
	}
};


class LightShader
{
public:

	void GetLightLocations(GLuint program)
	{
		lightposition = glGetUniformLocation(program, "lightposition");
	}

	void SetLightPosition(const glm::vec3& position) const
	{
		glUniform3fv(lightposition, 1, glm::value_ptr(position));
	}

	GLint lightposition;
};


class SimpleShader : public Shader, public ProjectionShader
{
public:

	using VertexType = VertexVC;

	SimpleShader()
	{
		auto simple_vertex_shader_resource = LOAD_RESOURCE(shader_simple_vertex_shader);
		auto simple_fragment_shader_resource = LOAD_RESOURCE(shader_simple_fragment_shader);

		CompileProgram(simple_vertex_shader_resource, simple_fragment_shader_resource);
		GetProjectionLocations(GetProgram());

		
	}
};


class PhongShader : public Shader, public ProjectionShader, public LightShader
{
public:
	
	PhongShader()
	{
		auto phong_vertex_shader_resource = LOAD_RESOURCE(shader_phong_vertex_shader);
		auto phong_fragment_shader_resource = LOAD_RESOURCE(shader_phong_fragment_shader);

		CompileProgram(phong_vertex_shader_resource, phong_fragment_shader_resource);
		GetProjectionLocations(GetProgram());
		GetLightLocations(GetProgram());
	}
};


class FontShader : public Shader, public ProjectionShader
{
public:

	using VertexType = VertexVT;

	FontShader()
	{
		auto font_vertex_shader_resource = LOAD_RESOURCE(shader_font_vertex_shader);
		auto font_fragment_shader_resource = LOAD_RESOURCE(shader_font_fragment_shader);

		CompileProgram(font_vertex_shader_resource, font_fragment_shader_resource);
		GetProjectionLocations(GetProgram());
		textColorLocation = glGetUniformLocation(GetProgram(), "texColor");
	}

	GLint textColorLocation;
};

class Shaders
{
public:
	Shaders()
	{
		auto simple_vertex_shader_resource = LOAD_RESOURCE(shader_simple_vertex_shader);
		auto simple_fragment_shader_resource = LOAD_RESOURCE(shader_simple_fragment_shader);
		auto font_vertex_shader_resource = LOAD_RESOURCE(shader_font_vertex_shader);
		auto font_fragment_shader_resource = LOAD_RESOURCE(shader_font_fragment_shader);
		//auto phong_vertex_shader_resource = LOAD_RESOURCE(shader_phong_vertex_shader);
		//auto phong_fragment_shader_resource = LOAD_RESOURCE(shader_phong_fragment_shader);

		shaders.push_back(Shader(simple_vertex_shader_resource.toString(), simple_fragment_shader_resource.toString(), std::string("Simple Shader")));
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

	size_t GetNumberShaders() const
	{
		return shaders.size();
	}

private:
	std::vector<Shader> shaders;
};

