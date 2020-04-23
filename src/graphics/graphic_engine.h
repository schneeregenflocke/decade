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

#pragma once

#include "shaders.h"
#include "projection.h"
#include "shapes.h"
#include "font.h"
#include "RenderToTexture.h"

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

	void SetRefreshCallback(wxGLCanvas* wx_gl_canvas);
	void Refresh();

/////////////////////////////////////

	void Render();
	void ProvisionalRenderToPNGRender();

	View& GetViewRef();

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

	void RenderToPNG(const std::wstring& file_path);
	void SlotPageSize(const std::array<float, 2> page_size);

private:

	View view;
	SimpleShader simple_shader;
	FontShader font_shader;

	std::vector< std::shared_ptr<ShapeBase> > shapes;

	wxGLCanvas* wx_gl_canvas;
	rect4 page_size;

/////////////////////////////////////	
};

