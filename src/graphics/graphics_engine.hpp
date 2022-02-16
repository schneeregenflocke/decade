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


#include "shaders.hpp"
#include "projection.hpp"
#include "shapes.hpp"
#include "font.hpp"
#include "mvp_matrices.hpp"

#include <algorithm>
#include <string>
#include <memory>
#include <exception>


class GraphicsEngine
{
public:

	void Render()
	{
		glClearColor(0.8f, 1.f, 0.8f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		simple_shader.UseProgram();
		simple_shader.SetProjectionMatrix(mvp.GetProjection());
		simple_shader.SetViewMatrix(mvp.GetView());
		simple_shader.SetModelMatrix(glm::mat4(1.f));

		font_shader.UseProgram();
		font_shader.SetProjectionMatrix(mvp.GetProjection());
		font_shader.SetViewMatrix(mvp.GetView());
		font_shader.SetModelMatrix(glm::mat4(1.f));

		for (size_t index = 0; index < shapes.size(); ++index)
		{
			shapes[index]->Draw();
		}
	}

	std::vector<std::string> GatherShapeNames()
	{
		std::vector<std::string> shape_names;
		for (const auto& shape : shapes)
		{
			shape_names.push_back(shape->GetShapeName());
		}
		return shape_names;
	}

	void SetMVP(const MVP& mvp)
	{
		this->mvp = mvp;
	}

	template<typename T, typename... Args>
	std::shared_ptr<T> AddShape(Args... args)
	{
		using ShaderTy = typename T::ShaderType;

		std::shared_ptr<T> shared_ptr = std::make_shared<T>(args...);
		
		if (std::is_same<ShaderTy, SimpleShader>::value)
		{
			shared_ptr->SetShader(&simple_shader);
		}

		if (std::is_same<ShaderTy, FontShader>::value)
		{
			shared_ptr->SetShader(&font_shader);
		}
		
		auto upcast_ptr = std::static_pointer_cast<ShapeBase>(shared_ptr);
		shapes.push_back(upcast_ptr);
		return shared_ptr;
	}

	template<typename T>
	void RemoveShape(std::shared_ptr<T> shape_ptr)
	{
		auto found = std::find(shapes.cbegin(), shapes.cend(), shape_ptr);
		if (found != shapes.cend())
		{
			shapes.erase(found);
			shape_ptr.reset();
		}
		else
		{
			throw std::invalid_argument("Shape not found, remove failed");
		}
	}

	template<typename T>
	void RemoveShapes(std::vector<std::shared_ptr<T>>& shape_ptrs)
	{
		for (size_t index = 0; index < shape_ptrs.size(); ++index)
		{
			RemoveShape(shape_ptrs[index]);
		}
		shape_ptrs.clear();
	}

private:

	MVP mvp;
	SimpleShader simple_shader;
	FontShader font_shader;

	std::vector<std::shared_ptr<ShapeBase>> shapes;
};
