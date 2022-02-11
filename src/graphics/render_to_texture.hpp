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


#include "texture_object.hpp"

#include <glad/glad.h>

#include <vector>


class RenderBuffer
{
public:

	explicit RenderBuffer()
	{
		glGenRenderbuffers(1, &name);
	}

	~RenderBuffer()
	{
		glDeleteRenderbuffers(1, &name);
	}

	GLuint Name() const
	{
		return name;
	}

private:

	GLuint name;
};


class FrameBuffer
{
public:

	explicit FrameBuffer(GLsizei width, GLsizei height, GLsizei samples, bool msaa)
	{
		glGenFramebuffers(1, &name);

		if (msaa == false)
		{
			glBindTexture(GL_TEXTURE_2D, texture.Name());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, render_buffer.Name());
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, name);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer.Name());
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.Name(), 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		
		if (msaa == true)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture.Name());
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, render_buffer.Name());
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, name);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture.Name(), 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer.Name());
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	~FrameBuffer()
	{
		glDeleteFramebuffers(1, &name);
	}

	GLenum CheckStatus() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, name);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return status;
	}

	GLuint Name() const
	{
		return name;
	}

	GLuint TextureName() const
	{
		return texture.Name();
	}

private:

	GLuint name;
	Texture texture;
	RenderBuffer render_buffer;
};


class RenderToTexture
{
public:
	
	RenderToTexture(GLsizei width, GLsizei height, GLsizei samples) :
		width(width),
		height(height),
		samples(samples),
		frame_buffer(width, height, samples, false),
		frame_buffer_msaa(width, height, samples, true),
		valid(false),
		pixel_size(0),
		restore_width(0),
		restore_height(0)
	{

		pixel_size = 4 * sizeof(GLubyte);

		/*int gl_max_texture_size = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);

		int gl_max_renderbuffer_size = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &gl_max_renderbuffer_size);

		int gl_max_samples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &gl_max_samples);

		int gl_max_viewport_dims = 0;
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &gl_max_viewport_dims);*/

		if (frame_buffer.CheckStatus() == GL_FRAMEBUFFER_COMPLETE && frame_buffer_msaa.CheckStatus() == GL_FRAMEBUFFER_COMPLETE)
		{
			valid = true;
		}
	}

	bool Valid()
	{
		return valid;
	}

	void BeginRender()
	{
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		restore_width = viewport[2];
		restore_height = viewport[3];

		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_msaa.Name());
		glViewport(0, 0, width, height);
	}

	void EndRender()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_msaa.Name());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer.Name());
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, restore_width, restore_height);
	}

	std::vector<unsigned char> CopyImage()
	{
		const size_t image_size = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(pixel_size);
		image.resize(image_size);

		glBindTexture(GL_TEXTURE_2D, frame_buffer.TextureName());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		return image;
	}

private:

	GLsizei width;
	GLsizei height;
	GLsizei samples;

	FrameBuffer frame_buffer;
	FrameBuffer frame_buffer_msaa;

	GLint restore_width;
	GLint restore_height;

	GLsizei pixel_size;
	std::vector<unsigned char> image;

	bool valid;
};
