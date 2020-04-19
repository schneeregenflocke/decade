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

#include "RenderToTexture.h"


FrameBufferObject::FrameBufferObject()
{
	glGenFramebuffers(1, &FBO);
}

FrameBufferObject::~FrameBufferObject()
{
	glDeleteFramebuffers(1, &FBO);
}

RenderBufferObject::RenderBufferObject()
{
	glGenRenderbuffers(1, &RBO);
}

RenderBufferObject::~RenderBufferObject()
{
	glDeleteRenderbuffers(1, &RBO);
}

void RenderBufferObject::SetRenderBufferObject(GLsizei width, GLsizei height)
{
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RenderBufferObject::SetRenderBufferObjectMultisample(GLsizei width, GLsizei height)
{
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}



void RenderTexture::Initialize(GLsizei width, GLsizei height)
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

void RenderTexture::BeginRender()
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	restorewidth = viewport[2];
	restoreheight = viewport[3];

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_msaa.FBO);
	glViewport(0, 0, width, height);
}

void RenderTexture::EndRender()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, restorewidth, restoreheight);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_msaa.FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.FBO);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void RenderTexture::GetPicture()
{
	pixelsize = 4 * sizeof(GLubyte);
	imagesize = width * height * pixelsize;

	image.resize(imagesize);

	//glGetTextureImage(tex.texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagesize, image.data());

	glBindTexture(GL_TEXTURE_2D, tex.Texture());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderTexture::SavePicture(const std::wstring& filename)
{
	VerticalFlip();

	lodepng::encode(std::string(filename.begin(), filename.end()), image.data(), width, height, LCT_RGBA);
}

void RenderTexture::VerticalFlip()
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


