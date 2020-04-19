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

#ifndef RENDERTOTEXTURE_H
#define RENDERTOTEXTURE_H


#include "texture_object.h"


//#define GLEW_STATIC
//#include <GL/glew.h>
#include <glad/glad.h>

#include <lodepng.h>

#include <iostream>
#include <exception>
#include <algorithm>
#include <string>
#include <thread>


class FrameBufferObject
{
public:
	FrameBufferObject();
	~FrameBufferObject();

	GLuint FBO;
};


class RenderBufferObject
{
public:
	RenderBufferObject();
	~RenderBufferObject();
	void SetRenderBufferObject(GLsizei width, GLsizei height);
	void SetRenderBufferObjectMultisample(GLsizei width, GLsizei height);

	GLuint RBO;
};





class RenderTexture
{
public:
	FrameBufferObject fbo, fbo_msaa;
	RenderBufferObject rbo, rbo_msaa;
	TextureObject tex, tex_msaa;

	void Initialize(GLsizei width, GLsizei height);

	void BeginRender();
	void EndRender();

	void GetPicture();
	void SavePicture(const std::wstring& filename);
	void VerticalFlip();

private:
	GLsizei width;
	GLsizei height;

	GLint restorewidth;
	GLint restoreheight;

	GLuint pixelsize;
	GLuint imagesize;

	std::vector<unsigned char> image;
};


#endif /* RENDERTOTEXTURE_H */
