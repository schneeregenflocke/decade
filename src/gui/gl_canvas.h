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

#include "../graphics/graphic_engine.h"

#ifdef WX_PRECOMP
#include <wx/wxprec.h>
#else 
#include <wx/wx.h>
#endif

#include <wx/glcanvas.h>

#include <memory>

//#include <glad/glad.h>
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>


class MouseInteraction
{
public:

    MouseInteraction(wxGLCanvas* parent, GraphicEngine* graphic_engine);
    void OnMouse(wxMouseEvent& event);

private:

    wxGLCanvas* parent;
    GraphicEngine* graphic_engine;

    glm::vec3 mouse_position_button_up;
    glm::vec3 move_view;
    glm::vec3 translate_view;
    glm::vec3 scale_view;
    bool switch_to_button_up;
    float scale;
};


class GLCanvas : public wxGLCanvas
{
public:

    GLCanvas(wxWindow* parent, const wxGLAttributes& canvas_attributes);
    const wxEventTypeTag<wxCommandEvent> GetGLReadyEventTag();
    GraphicEngine* GetGraphicEngine();

    void LoadOpenGL();

private:

    std::unique_ptr<wxGLContext> context;
	wxGLContextAttrs context_attributes;
    //wxGLContextAttrs defaultAttrs();
    

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    


    int openGL_ready;
    std::unique_ptr<GraphicEngine> graphic_engine;

    const wxEventTypeTag<wxCommandEvent> wxCommandEventTag;

    std::unique_ptr<MouseInteraction> mouse_interaction;
};
