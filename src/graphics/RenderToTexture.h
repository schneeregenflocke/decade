/*
Decade
Copyright (c) 2019-2020 Marco Peyer

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
	FrameBufferObject()
	{
		glGenFramebuffers(1, &FBO);
	}
	~FrameBufferObject()
	{
		glDeleteFramebuffers(1, &FBO);
	}

	GLuint FBO;
};


class RenderBufferObject
{
public:
	RenderBufferObject()
	{
		glGenRenderbuffers(1, &RBO);
	}
	~RenderBufferObject()
	{
		glDeleteRenderbuffers(1, &RBO);
	}
	void SetRenderBufferObject(GLsizei width, GLsizei height)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	void SetRenderBufferObjectMultisample(GLsizei width, GLsizei height)
	{
		glBindRenderbuffer(GL_RENDERBUFFER, RBO);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	GLuint RBO;
};





class RenderTexture
{
public:
	FrameBufferObject fbo, fbo_msaa;
	RenderBufferObject rbo, rbo_msaa;
	TextureObject tex, tex_msaa;

	void Initialize(GLsizei width, GLsizei height)
	{
		this->width = width;
		this->height = height;

		rbo.SetRenderBufferObject(width, height);
		rbo_msaa.SetRenderBufferObjectMultisample(width, height);

		tex.SetTexture2D(width, height);
		tex_msaa.SetTexture2DMultisample(width, height);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo.FBO);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo.RBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.Texture(), 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo_msaa.FBO);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_msaa.RBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex_msaa.Texture(), 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void BeginRender()
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		restorewidth = viewport[2];
		restoreheight = viewport[3];

		glBindFramebuffer(GL_FRAMEBUFFER, fbo_msaa.FBO);
		glViewport(0, 0, width, height);
	}
	void EndRender()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, restorewidth, restoreheight);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_msaa.FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.FBO);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GetPicture()
	{
		pixelsize = 4 * sizeof(GLubyte);
		imagesize = width * height * pixelsize;

		image.resize(imagesize);

		//glGetTextureImage(tex.texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagesize, image.data());

		glBindTexture(GL_TEXTURE_2D, tex.Texture());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void SavePicture(const std::wstring& filename)
	{
		VerticalFlip();

		lodepng::encode(std::string(filename.begin(), filename.end()), image.data(), width, height, LCT_RGBA);
	}
	void VerticalFlip()
	{
		std::vector<unsigned char> buffer(image);

		for (auto index = 0; index < height; ++index)
		{
			auto rowsize = width * pixelsize;
			auto subindex0 = index * rowsize;
			auto subindex1 = subindex0 + rowsize;

			std::copy(buffer.cbegin() + subindex0, buffer.cbegin() + subindex1, image.end() - subindex1);
		}
	}

private:
	GLsizei width;
	GLsizei height;

	GLint restorewidth;
	GLint restoreheight;

	GLuint pixelsize;
	GLuint imagesize;

	std::vector<unsigned char> image;
};
