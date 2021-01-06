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

#include "shaders.h"
#include "projection.h"
#include "shapes.h"
#include "font.h"
#include "RenderToTexture.h"

#include "../packages/page_config.h"

#include <glm/ext/matrix_projection.hpp>

#include <iostream>
#include <algorithm>
#include <type_traits>
#include <algorithm>
#include <string>
#include <memory>

#include <wx/glcanvas.h>


class GraphicEngine
{
public:

/////////////////////////////////////

	void SetRefreshCallback(wxGLCanvas* wx_gl_canvas)
	{
		this->wx_gl_canvas = wx_gl_canvas;
	}
	void Refresh()
	{
		wx_gl_canvas->Refresh(false);
	}

/////////////////////////////////////

	void Render()
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
	void ProvisionalRenderToPNGRender()
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

	View& GetViewRef()
	{
		return view;
	}

	template<typename T> 
	std::shared_ptr<T> AddShape()
	{
		auto ptr = std::make_shared<T>();
		auto upcast = std::static_pointer_cast<ShapeBase>(ptr);

		using U = typename T::ShaderType;
		
		if (std::is_same<U, SimpleShader>::value)
		{
			upcast->SetShader(&simple_shader);
		}

		if (std::is_same<U, FontShader>::value)
		{
			upcast->SetShader(&font_shader);
		}
		
		shapes.push_back(upcast);
		return ptr;
	}

	template<typename T>
	std::vector< std::shared_ptr<T> > AddShapes(size_t number)
	{
		std::vector< std::shared_ptr<T> > temp;

		for (size_t index = 0; index < number; ++index)
		{
			temp.push_back(AddShape<T>());
		}

		return temp;
	}

	template<typename T>
	void RemoveShape(std::shared_ptr<T> shape_ptr)
	{
		auto found = std::find(shapes.cbegin(), shapes.cend(), shape_ptr);
		if (found != shapes.cend())
		{
			shapes.erase(found);
			//shape_ptr.reset();
		}
	}

	template<typename T>
	void RemoveShapes(const std::vector< std::shared_ptr<T> > shape_ptrs)
	{
		for (size_t index = 0; index < shape_ptrs.size(); ++index)
		{
			RemoveShape(shape_ptrs[index]);
		}
	}

	void RenderToPNG(const std::wstring& file_path)
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

	void ReceivePageSetup(const PageSetupConfig& page_setup_config)
	{
		this->page_size = rect4(page_setup_config.size[0], page_setup_config.size[1]);
	}

private:

	View view;
	SimpleShader simple_shader;
	FontShader font_shader;

	std::vector< std::shared_ptr<ShapeBase> > shapes;

	wxGLCanvas* wx_gl_canvas;
	rect4 page_size;

/////////////////////////////////////	
};
