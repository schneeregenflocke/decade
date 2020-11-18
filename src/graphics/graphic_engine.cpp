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


#include "graphic_engine.h"


void GraphicEngine::SetRefreshCallback(wxGLCanvas* wx_gl_canvas)
{
	this->wx_gl_canvas = wx_gl_canvas;
}

void GraphicEngine::Refresh()
{
	wx_gl_canvas->Refresh(false);
}


void GraphicEngine::Render()
{
	glClearColor(.9F, 1.F, .9F, 1.0F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	simple_shader.UseProgram();
	simple_shader.SetProjectionMatrix(view.GetProjectionMatrix());
	simple_shader.SetViewMatrix(view.GetViewMatrix());
	simple_shader.SetModelMatrix(glm::mat4(1.F));

	font_shader.UseProgram();
	font_shader.SetProjectionMatrix(view.GetProjectionMatrix());
	font_shader.SetViewMatrix(view.GetViewMatrix());
	font_shader.SetModelMatrix(glm::mat4(1.f));

	
	for (size_t index = 0; index < shapes.size(); ++index)
	{
		shapes[index]->Draw();
	}
}

void GraphicEngine::ProvisionalRenderToPNGRender()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	simple_shader.UseProgram();
	simple_shader.SetProjectionMatrix(Projection::OrthoMatrix(page_size));
	simple_shader.SetViewMatrix(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)));
	simple_shader.SetModelMatrix(glm::mat4(1.f));

	font_shader.UseProgram();
	font_shader.SetProjectionMatrix(Projection::OrthoMatrix(page_size));
	font_shader.SetViewMatrix(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, 0.f)));
	font_shader.SetModelMatrix(glm::mat4(1.f));


	for (size_t index = 0; index < shapes.size(); ++index)
	{
		shapes[index]->Draw();
	}
}


View& GraphicEngine::GetViewRef()
{
	return view;
}




void GraphicEngine::RenderToPNG(const std::wstring& file_path)
{

/////////////////////////////////////
	int multiplier = 16;

/////////////////////////////////////
	GLsizei pixel_width = static_cast<GLsizei>(page_size.Width());
	GLsizei pixel_height = static_cast<GLsizei>(page_size.Height());

	RenderTexture renderToPng;
	renderToPng.Initialize(pixel_width * multiplier, pixel_height * multiplier);
	renderToPng.BeginRender();

	ProvisionalRenderToPNGRender();

	renderToPng.EndRender();
	renderToPng.GetPicture();
	renderToPng.SavePicture(file_path);
}

