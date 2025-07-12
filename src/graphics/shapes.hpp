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
GNU General Public Licens	e for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "rect.hpp"
#include "shaders.hpp"
#include "shapes_base.hpp"

// #define GLM_FORCE_MESSAGES
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

// #include <array>
// #include <cmath>
#include <vector>

class QuadrilateralShape : public Shape {
public:
  QuadrilateralShape(Shader *shader_ptr) : Shape(shader_ptr) {}

  void set_shape(const rectf &rectangle)
  {
    constexpr size_t number_vertices = 6;
    std::vector<glm::vec3> vertices(number_vertices);

    vertices[0] = glm::vec3(rectangle.l(), rectangle.b(), 0.f);
    vertices[1] = glm::vec3(rectangle.r(), rectangle.b(), 0.f);
    vertices[2] = glm::vec3(rectangle.l(), rectangle.t(), 0.f);
    vertices[3] = glm::vec3(rectangle.r(), rectangle.t(), 0.f);
    vertices[4] = glm::vec3(rectangle.l(), rectangle.t(), 0.f);
    vertices[5] = glm::vec3(rectangle.r(), rectangle.b(), 0.f);

    set_buffer(0, vertices.size(), vertices.data());
  }

  void set_color(const glm::vec4 &color) { this->color = color; }

  virtual void draw() const override
  {
    shader_ptr->UseProgram();
    shader_ptr->SetUniform("color", color);

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices);
    vao.unbind();
  }

private:
  glm::vec4 color;
};

class RectanglesShape : public Shape {
public:
  RectanglesShape(Shader *shader_ptr) : Shape(shader_ptr) {}

  void set_shape(const std::vector<rectf> &rectangles, float line_width)
  {
    constexpr size_t vertices_per_rectangle = 6 * 5;
    size_t size = rectangles.size() * vertices_per_rectangle;
    vertices.resize(size);

    for (size_t index = 0; index < rectangles.size(); ++index) {
      set_rectangle_shape(index, rectangles[index], line_width);
    }

    set_buffer(0, vertices.size(), vertices.data());
  }

  void set_shape(const rectf &rectangle, float line_width)
  {
    constexpr size_t vertices_per_rectangle = 6 * 5;
    vertices.resize(vertices_per_rectangle);

    set_rectangle_shape(0, rectangle, line_width);

    set_buffer(0, vertices.size(), vertices.data());
  }

  void set_color(const std::vector<glm::vec4> &colors) { this->colors = colors; }

  virtual void draw() const override
  {

    shader_ptr->UseProgram();

    if (colors.size() == 2) {
      shader_ptr->SetUniform("outline_color", colors[0]);
      shader_ptr->SetUniform("fill_color", colors[1]);
    }

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, number_vertices);
    vao.unbind();
  }

private:
  void set_rectangle_shape(size_t index, const rectf &rectangle, float line_width)
  {
    float hlt = line_width * 0.5f;

    rectf inrectangle = rectangle.reduce(rectf(hlt, hlt, hlt, hlt));
    rectf outrectangle = rectangle.expand(rectf(hlt, hlt, hlt, hlt));

    size_t offset = index * 6 * 5;

    // fill
    set_rectangle(offset + 0 * 6, inrectangle.getLB(), inrectangle.getRB(), inrectangle.getLT(),
                  inrectangle.getRT());
    // top outline
    set_rectangle(offset + 1 * 6, inrectangle.getLT(), inrectangle.getRT(), outrectangle.getLT(),
                  outrectangle.getRT());
    // bottom outline
    set_rectangle(offset + 2 * 6, outrectangle.getLB(), outrectangle.getRB(), inrectangle.getLB(),
                  inrectangle.getRB());
    // left outline
    set_rectangle(offset + 3 * 6, outrectangle.getLB(), inrectangle.getLB(), outrectangle.getLT(),
                  inrectangle.getLT());
    // right outline
    set_rectangle(offset + 4 * 6, inrectangle.getRB(), outrectangle.getRB(), inrectangle.getRT(),
                  outrectangle.getRT());
  }

  void set_rectangle(size_t offset, const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2,
                     const glm::vec3 &p3)
  {
    vertices[offset + 0] = p0;
    vertices[offset + 1] = p1;
    vertices[offset + 2] = p2;
    vertices[offset + 3] = p3;
    vertices[offset + 4] = p2;
    vertices[offset + 5] = p1;
  }

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec4> colors;
};

/*class OrthoLineShape : public Shape<SimpleShader>
{
public:

        explicit OrthoLineShape(const std::string& shape_name) :
                Shape<SimpleShader>(shape_name)
        {
                SetBufferSize(6);
        }

        void SetShape(const glm::vec3& p0, const glm::vec3& p1, float width)
        {
                auto halfwidth = width / 2.f;

                glm::vec3 hvec0(-halfwidth, -halfwidth, 0.f);
                glm::vec3 hvec1(halfwidth, -halfwidth, 0.f);
                glm::vec3 hvec2(-halfwidth, halfwidth, 0.f);
                glm::vec3 hvec3(halfwidth, halfwidth, 0.f);

                auto qp0 = p0 + hvec0;
                auto qp1 = p1 + hvec1;
                auto qp2 = p0 + hvec2;
                auto qp3 = p1 + hvec3;

                GetVertexRef(0).point = qp0;
                GetVertexRef(1).point = qp1;
                GetVertexRef(2).point = qp2;
                GetVertexRef(3).point = qp1;
                GetVertexRef(4).point = qp3;
                GetVertexRef(5).point = qp2;

                UpdateBuffer();
        }
};*/

/*class LineShape : public Shape<SimpleShader>
{
public:

        explicit LineShape(const std::string& shape_name) :
                Shape<SimpleShader>(shape_name)
        {
                SetBufferSize(6);
        }

        void SetShape(const glm::vec3& p0, const glm::vec3& p1, float width)
        {
                auto direction = p1 - p0;
                auto normdirection = glm::normalize(direction);
                auto halfwidth = width / 2.f;
                auto scalednormdirection = normdirection * halfwidth;

                glm::vec3 hvec0(scalednormdirection.y, -scalednormdirection.x, 0.f); // CW
                glm::vec3 hvec1(-scalednormdirection.y, scalednormdirection.x, 0.f); // CCW

                auto qp0 = p0 + hvec0;
                auto qp1 = p1 + hvec0;
                auto qp2 = p0 + hvec1;
                auto qp3 = p1 + hvec1;

                GetVertexRef(0).point = qp0;
                GetVertexRef(1).point = qp1;
                GetVertexRef(2).point = qp2;
                GetVertexRef(3).point = qp3;
                GetVertexRef(4).point = qp2;
                GetVertexRef(5).point = qp1;

                UpdateBuffer();
        }
};*/

/*class CuboidShape : public Shape<SimpleShader>
{
public:

        explicit CuboidShape(const std::string& shape_name) :
                Shape<SimpleShader>(shape_name)
        {
                SetBufferSize(36);
        }

        void SetShape(const glm::vec3& center, float width, float height, float depth)
        {
                float halfwidth = width * 0.5f;
                float halfheight = height * 0.5f;
                float halfdepth = depth * 0.5f;

                std::array<glm::vec3, 8> points;

                points[0] = center + glm::vec3(-halfwidth, -halfheight, halfdepth);
                points[1] = center + glm::vec3(halfwidth, -halfheight, halfdepth);
                points[2] = center + glm::vec3(-halfwidth, halfheight, halfdepth);
                points[3] = center + glm::vec3(halfwidth, halfheight, halfdepth);

                points[4] = center + glm::vec3(halfwidth, -halfheight, -halfdepth);
                points[5] = center + glm::vec3(-halfwidth, -halfheight, -halfdepth);
                points[6] = center + glm::vec3(halfwidth, halfheight, -halfdepth);
                points[7] = center + glm::vec3(-halfwidth, halfheight, -halfdepth);

                std::array<size_t, 36> indices = {
                        0, 1, 2, 3, 2, 1, // front quad
                        4, 5, 6, 7, 6, 5, // back quad
                        1, 4, 3, 6, 3, 4, // right quad
                        5, 0, 7, 2, 7, 0, // left quad
                        2, 3, 7, 6, 7, 3, // top quad
                        5, 4, 0, 1, 0, 4  // bottom quad
                };

                for (auto index = 0U; index < 36; ++index)
                {
                        GetVertexRef(index).point = points[indices[index]];
                }

                UpdateBuffer();
        }
};*/

/*class Triangle
{
public:
        explicit Triangle(Shape *shape, const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3
&p2)
        {
                std::vector<glm::vec3> points = {p0, p1, p2};
                shape->set_buffer(0, points);
        }
private:
};*/

/*class CircleBase
{
public:
        explicit CircleBase()
        {
                SetResolution(64);
        }

        void SetResolution(size_t resolution)
        {
                //SetBufferSize(resolution * 3);
                this->resolution = resolution;
        }

protected:

        static glm::vec3 CirclePoint(float angle, float radius)
        {
                return glm::vec3(glm::cos(angle) * radius, glm::sin(angle) * radius, 0.f);
        }

        size_t resolution;
};*/

/*class CircleShape : public CircleBase
{
public:

        explicit CircleShape(Shape* shape, const glm::vec3& position, float radius, glm::vec4 color)
        {


        }

        void SetShape(const glm::vec3& position, float radius, glm::vec4 color)
        {
                float step = glm::two_pi<float>() / static_cast<float>(resolution);
                auto p0 = glm::vec3(0.f, 0.f, 0.f);
                auto p1 = CirclePoint(0.f, radius);
                auto p2 = CirclePoint(step, radius);

                std::vector<glm::vec3> points(resolution);

                for (size_t index = 0; index < resolution; ++index)
                {
                        auto angle = step * static_cast<float>(index);

                        auto rp0 = glm::rotateZ(p0, angle);
                        auto rp1 = glm::rotateZ(p1, angle);
                        auto rp2 = glm::rotateZ(p2, angle);

                        size_t subindex = index * 3;
                        GetVertexRef(subindex + 0).point = rp0 + position;
                        GetVertexRef(subindex + 1).point = rp1 + position;
                        GetVertexRef(subindex + 2).point = rp2 + position;

                        GetVertexRef(subindex + 0).color = color;
                        GetVertexRef(subindex + 1).color = color;
                        GetVertexRef(subindex + 2).color = color;
                }
                // UpdateBuffer();
        }
};*/

/*class CircleSectorShape : public CircleBase
{
public:

        explicit CircleSectorShape(const std::string& shape_name) :
                CircleBase(shape_name)
        {}

        void SetShape(const glm::vec3& position, float radius, float sector_begin, float sector_end)
        {
                float step = (sector_begin - sector_end) / static_cast<float>(resolution);

                auto p0 = glm::vec3(0.f, 0.f, 0.f);
                auto p1 = CirclePoint(sector_begin, radius);
                auto p2 = CirclePoint(sector_begin + step, radius);

                for (size_t index = 0; index < resolution; ++index)
                {
                        auto angle = step * static_cast<float>(index);
                        auto rp0 = glm::rotateZ(p0, angle);
                        auto rp1 = glm::rotateZ(p1, angle);
                        auto rp2 = glm::rotateZ(p2, angle);

                        auto subindex = index * 3U;
                        GetVertexRef(subindex + 0).point = rp0 + position;
                        GetVertexRef(subindex + 1).point = rp1 + position;
                        GetVertexRef(subindex + 2).point = rp2 + position;
                }
                UpdateBuffer();
        }
};*/
