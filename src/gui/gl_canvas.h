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

#include <glad/glad.h>

#include "../graphics/graphic_engine.h"


#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/glcanvas.h>
#include <wx/dcclient.h>

#include <sigslot/signal.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/io.hpp>

#include <memory>
#include <array>
#include <string>


class MouseInteraction
{
public:

    MouseInteraction(wxGLCanvas* parent, GraphicEngine* graphic_engine) :
        parent(parent),
        graphic_engine(graphic_engine),
        previous_mouse_pos(0.f),
        scale_factor(1.f),
        translate_pre_scaled(0.f),
        translate_post_scaled(0.f)
    {}

    void OnMouse(wxMouseEvent& event)
    {
        auto should_skip = event.GetSkipped();
        event.Skip(should_skip);

        glm::vec3 current_mouse_pos = MouseWorldSpacePos(event.GetPosition());

        if (event.Dragging())
        {
            translate_pre_scaled += current_mouse_pos - previous_mouse_pos;
        }

        if (event.GetWheelRotation() != 0)
        {
            const float mouse_wheel_step = 1200.f;
            const int wheel_rotation = event.GetWheelRotation();
            const auto scale = static_cast<float>(wheel_rotation) / mouse_wheel_step;
            
            const auto pre_scale_mouse_pos = MouseViewSpacePos(current_mouse_pos, CalculateViewMatrix());

            scale_factor *= std::exp(scale);

            const auto post_scale_mouse_pos = MouseViewSpacePos(current_mouse_pos, CalculateViewMatrix());

            const glm::vec3 view_space_correction = post_scale_mouse_pos - pre_scale_mouse_pos;

            translate_post_scaled += view_space_correction;
        }

        previous_mouse_pos = current_mouse_pos;

        graphic_engine->GetViewRef().SetViewMatrix(CalculateViewMatrix());

        parent->Refresh(false);
    }

private:

    wxGLCanvas* parent;
    GraphicEngine* graphic_engine;

    glm::vec3 previous_mouse_pos;

    glm::vec3 translate_pre_scaled;
    glm::vec3 translate_post_scaled;
    float scale_factor;

    glm::mat4 CalculateViewMatrix()
    {
        auto pre_scaled = glm::translate(glm::mat4(1.f), translate_pre_scaled);
        auto post_scaled = glm::scale(pre_scaled, glm::vec3(scale_factor, scale_factor, 1.f));
        return glm::translate(post_scaled, translate_post_scaled);
    }

    glm::vec3 MouseClipSpace(const wxPoint& mouse_pos_px)
    {
        const glm::vec2 window_mouse_pos(static_cast<float>(mouse_pos_px.x), static_cast<float>(mouse_pos_px.y));

        std::array<GLint, 4> viewport_px;
        glGetIntegerv(GL_VIEWPORT, viewport_px.data());
        glm::vec4 viewport(0.f, 0.f, static_cast<float>(viewport_px[2]), static_cast<float>(viewport_px[3]));

        auto viewport_ortho = glm::ortho(viewport.x, viewport.z, viewport.w, viewport.y);
        auto mouse_pos_clip_space = viewport_ortho * glm::vec4(window_mouse_pos, 0.f, 1.f);
        return mouse_pos_clip_space;
    }

    glm::vec3 MouseWorldSpacePos(const wxPoint& mouse_pos_px)
    {
        const glm::mat4 projection_matrix = graphic_engine->GetViewRef().GetProjectionMatrix();
        const auto inverse_projection_matrix = glm::inverse(projection_matrix);

        const auto mouse_pos = inverse_projection_matrix * glm::vec4(MouseClipSpace(mouse_pos_px), 1.f);
        return glm::vec3(mouse_pos.x, mouse_pos.y, 0.f);
    }

    glm::vec3 MouseViewSpacePos(const glm::vec3& mouse_world_space_pos, const glm::mat4& view_matrix)
    {
        const auto inverse_view_matrix = glm::inverse(view_matrix);

        const auto mouse_pos = inverse_view_matrix * glm::vec4(mouse_world_space_pos, 1.f);
        return glm::vec3(mouse_pos.x, mouse_pos.y, 0.f);
    }
};


class GLCanvas : public wxGLCanvas
{
public:

    GLCanvas(wxWindow* parent, const wxGLAttributes& canvas_attributes) :
        wxGLCanvas(parent, canvas_attributes),
        openGL_ready(0)
    {}

    GraphicEngine* GetGraphicEngine()
    {
        return graphic_engine.get();
    }

    void LoadOpenGL(const std::array<int, 2>& version)
    {
        bool canvas_shown_on_screen = this->IsShownOnScreen();

        if (!canvas_shown_on_screen)
        {
            std::cerr << "Try wxFrame::Raise after WxFrame::Show" << '\n';
        }
        if (openGL_ready == 0 && canvas_shown_on_screen)
        {
            context_attributes.PlatformDefaults().CoreProfile().OGLVersion(version[0], version[1]).EndList();
            context = std::make_unique<wxGLContext>(this, nullptr, &context_attributes);
            std::cout << "context IsOK " << context->IsOK() << '\n';

            SetCurrent(*context);
            openGL_ready = gladLoadGL();

            std::cout << "OpenGL ready: " << openGL_ready << " version: " << GetGLVersionString() << '\n';

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_MULTISAMPLE);

            GLint sample_buffers = 0;
            glGetIntegerv(GL_SAMPLES, &sample_buffers);
            std::cout << "sample_buffers " << sample_buffers << '\n';

            graphic_engine = std::make_unique<GraphicEngine>();

            graphic_engine->SetRefreshCallback(this);

            Bind(wxEVT_SIZE, &GLCanvas::OnSize, this);
            Bind(wxEVT_PAINT, &GLCanvas::OnPaint, this);

            mouse_interaction = std::make_unique<MouseInteraction>(this, graphic_engine.get());

            Bind(wxEVT_MOTION, &MouseInteraction::OnMouse, mouse_interaction.get());
            Bind(wxEVT_LEFT_DOWN, &MouseInteraction::OnMouse, mouse_interaction.get());
            Bind(wxEVT_LEFT_UP, &MouseInteraction::OnMouse, mouse_interaction.get());
            Bind(wxEVT_MOUSEWHEEL, &MouseInteraction::OnMouse, mouse_interaction.get());

            signal_opengl_ready();
        }
    }
    static std::string GetGLVersionString()
    {
        std::string version_string;
        version_string += "GL_VERSION " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VERSION))) + '\n';
        version_string += "GL_VENDOR " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VENDOR))) + '\n';
        version_string += "GL_RENDERER " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        return version_string;
    }

    sigslot::signal<> signal_opengl_ready;

private:

    std::unique_ptr<wxGLContext> context;
	wxGLContextAttrs context_attributes;

    void OnPaint(wxPaintEvent& event)
    {
        event.Skip();

        wxPaintDC dc(this);

        graphic_engine->Render();

        SwapBuffers();
    }
    void OnSize(wxSizeEvent& event)
    {
        event.Skip();

        wxSize size = GetClientSize();
        glViewport(0, 0, size.GetWidth(), size.GetHeight());

        //graphic_engine->GetViewRef().UpdateOrthoMatrix();

        Refresh(false);
    }
    
    int openGL_ready;

    std::unique_ptr<GraphicEngine> graphic_engine;
    std::unique_ptr<MouseInteraction> mouse_interaction;
};
