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

#include "shaders.h"


void VertexVC::SetAttribPointers()
{
	GLsizei stride = sizeof(VertexVC);

	auto offsetof0 = offsetof(VertexVC, point);
	auto offsetof1 = offsetof(VertexVC, color);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof0));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof1));
}


void VertexVC::EnableVertexAttribArrays()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}


void VertexVC::DisableVertexAttribArrays()
{
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}


void VertexVT::SetAttribPointers()
{
	GLsizei stride = sizeof(VertexVT);

	//auto offsetof0 = offsetof(VertexVT, position);
	//auto offsetof1 = offsetof(VertexVT, texturePosition);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(VertexVT, position)));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(VertexVT, texturePosition)));
}


void VertexVT::EnableVertexAttribArrays()
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}


void VertexVT::DisableVertexAttribArrays()
{
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}




void Shader::CompileProgram(Resource vertexshaderfile, Resource fragmentshaderfile)
{
	GLuint vertexshader = CompileShader(vertexshaderfile, GL_VERTEX_SHADER);
	GLuint fragmentshader = CompileShader(fragmentshaderfile, GL_FRAGMENT_SHADER);
	program = LinkShaders(vertexshader, fragmentshader);
}


void Shader::UseProgram()
{
	glUseProgram(program);
}

GLuint Shader::GetProgram()
{
	return program;
}


void ProjectionShader::GetProjectionLocations(GLuint program)
{
	model = glGetUniformLocation(program, "model");
	view = glGetUniformLocation(program, "view");
	projection = glGetUniformLocation(program, "projection");
}


void ProjectionShader::SetViewMatrix(const mat4& value) const
{
	glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(value));
}


void ProjectionShader::SetModelMatrix(const mat4& value) const
{
	glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(value));
}


void ProjectionShader::SetProjectionMatrix(const mat4& value) const
{
	glUniformMatrix4fv(projection, 1, GL_FALSE, glm::value_ptr(value));
}


void LightShader::GetLightLocations(GLuint program)
{
	lightposition = glGetUniformLocation(program, "lightposition");
}


void LightShader::SetLightPosition(const vec3& position) const
{
	glUniform3fv(lightposition, 1, glm::value_ptr(position));
}


SimpleShader::SimpleShader()
{
	auto simple_vertex_shader_resource = LOAD_RESOURCE(shaders_simple_vertex_shader_glsl);
	auto simple_fragment_shader_resource = LOAD_RESOURCE(shaders_simple_fragment_shader_glsl);

	CompileProgram(simple_vertex_shader_resource, simple_fragment_shader_resource);
	GetProjectionLocations(GetProgram());
}


PhongShader::PhongShader()
{
	//CompileProgram("VertexShaderPhong.glsl", "FragmentShaderPhong.glsl");
	GetProjectionLocations(GetProgram());
	GetLightLocations(GetProgram());
}

FontShader::FontShader()
{
	auto font_vertex_shader_resource = LOAD_RESOURCE(shaders_font_vertex_shader_glsl);
	auto font_fragment_shader_resource = LOAD_RESOURCE(shaders_font_fragment_shader_glsl);

	CompileProgram(font_vertex_shader_resource, font_fragment_shader_resource);
	GetProjectionLocations(GetProgram());
	textColorLocation = glGetUniformLocation(GetProgram(), "texColor");
}


GLuint Shader::CompileShader(const Resource& resource, const GLuint shadertype)
{
	GLuint simple_shader = glCreateShader(shadertype);
	const char* sourcepointer = resource.data();
	glShaderSource(simple_shader, 1, &sourcepointer, NULL);
	glCompileShader(simple_shader);

	GLint compilestatus;
	glGetShaderiv(simple_shader, GL_COMPILE_STATUS, &compilestatus);
	std::cout << "GL_COMPILE_STATUS " << simple_shader << " " << std::boolalpha << static_cast<bool>(compilestatus) << std::noboolalpha << '\n';

	GLint infolenght;
	glGetShaderiv(simple_shader, GL_INFO_LOG_LENGTH, &infolenght);

	if (infolenght > 0)
	{
		char* infolog = new char[infolenght];
		glGetShaderInfoLog(simple_shader, infolenght, NULL, infolog);
		std::cout << infolog;
		delete[] infolog;
	}

	return simple_shader;
}


GLuint Shader::LinkShaders(const GLuint vertexshader, const GLuint fragmentshader)
{
	GLuint program = glCreateProgram();

	glAttachShader(program, vertexshader);
	glAttachShader(program, fragmentshader);

	glLinkProgram(program);

	GLint linkstatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkstatus);
	std::cout << "GL_LINK_STATUS " << program << " " << std::boolalpha << static_cast<bool>(linkstatus) << std::noboolalpha << '\n';

	GLint infolenght;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infolenght);

	if (infolenght > 0)
	{
		char* infolog = new char[infolenght];
		glGetProgramInfoLog(program, infolenght, NULL, infolog);
		std::cout << infolog;
		delete[] infolog;
	}

	glDetachShader(program, vertexshader);
	glDetachShader(program, fragmentshader);
	glDeleteShader(vertexshader);
	glDeleteShader(fragmentshader);

	return program;
}


