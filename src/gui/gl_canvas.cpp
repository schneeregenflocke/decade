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

#include "gl_canvas.h"


GLCanvas::GLCanvas(wxWindow* parent, const wxGLAttributes& canvas_attributes) : 
    wxGLCanvas(parent, canvas_attributes),
    wxCommandEventTag(wxNewEventType()),
    openGL_ready(false)
{
	/*	/home/titan99/code/decade/src/gui/gl_canvas.cpp: In Konstruktor »GLCanvas::GLCanvas(wxWindow*, const wxGLAttributes&)«:
		/home/titan99/code/decade/src/gui/gl_canvas.cpp:24:42: Fehler: Adresse eines rvalues wird ermittelt [-fpermissive]
   		24 |     context(this, nullptr, &defaultAttrs()),
	*/
	
	/*context_attributes.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
	context = new wxGLContext(this, nullptr, &context_attributes);
	std::cout << "context IsOK " << context->IsOK() << '\n';*/

    //Bind(wxEVT_PAINT, &GLCanvas::OnPaint, this);
    //Bind(wxEVT_SIZE, &GLCanvas::OnSize, this);


}

const wxEventTypeTag<wxCommandEvent> GLCanvas::GetGLReadyEventTag()
{
    return wxCommandEventTag;
}

GraphicEngine* GLCanvas::GetGraphicEngine()
{
    return graphic_engine.get();
}

/*wxGLContextAttrs GLCanvas::defaultAttrs()
{
    context_attributes.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
    return context_attributes;
}*/


void GLCanvas::LoadOpenGL()
{
    bool shown_on_screen = GetParent()->IsShownOnScreen();
    bool canvas_shown_on_screen = this->IsShownOnScreen();

	if (openGL_ready == false && shown_on_screen)
    {
		context_attributes.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
		context = std::make_unique<wxGLContext>(this, nullptr, &context_attributes);
		std::cout << "context IsOK " << context->IsOK() << '\n';

        SetCurrent(*context);
        openGL_ready = gladLoadGL();

        wxString version_string;
        version_string += "__cplusplus " + std::to_string(__cplusplus) + '\n';
        version_string += "GL_VERSION " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VERSION))) + '\n';
        version_string += "GL_VENDOR " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VENDOR))) + '\n';
        version_string += "GL_RENDERER " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
        std::cout << version_string << '\n';

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE);

        GLint sample_buffers;
        glGetIntegerv(GL_SAMPLES, &sample_buffers);
        std::cout << "sample_buffers " << sample_buffers << '\n';

        graphic_engine = std::make_unique<GraphicEngine>();
        graphic_engine->SetRefreshCallback(this);

        Bind(wxEVT_SIZE, &GLCanvas::OnSize, this);
        Bind(wxEVT_PAINT, &GLCanvas::OnPaint, this);

        wxCommandEvent openGL_ready_event;
        openGL_ready_event.SetEventType(wxCommandEventTag);
        ProcessWindowEvent(openGL_ready_event);

        mouse_interaction = std::make_unique<MouseInteraction>(this, graphic_engine.get());

        Bind(wxEVT_MOTION, &MouseInteraction::OnMouse, mouse_interaction.get());
        Bind(wxEVT_LEFT_DOWN, &MouseInteraction::OnMouse, mouse_interaction.get());
        Bind(wxEVT_LEFT_UP, &MouseInteraction::OnMouse, mouse_interaction.get());
        Bind(wxEVT_MOUSEWHEEL, &MouseInteraction::OnMouse, mouse_interaction.get());
    }
}


void GLCanvas::OnPaint(wxPaintEvent& event)
{
    event.Skip();

    wxPaintDC dc(this);

    graphic_engine->Render();

    SwapBuffers();
}

void GLCanvas::OnSize(wxSizeEvent& event)
{
    event.Skip();

    wxSize size = GetClientSize();
    glViewport(0, 0, size.GetWidth(), size.GetHeight());

    Refresh(false);  
}


MouseInteraction::MouseInteraction(wxGLCanvas* parent, GraphicEngine* graphic_engine) : 
    parent(parent), 
    graphic_engine(graphic_engine)
{
    translate_view = glm::vec3(0.f);
    move_view = glm::vec3(0.f);
    switch_to_button_up = false;
    scale_view = glm::vec3(1.f);
    scale = 0.f;
}

void MouseInteraction::OnMouse(wxMouseEvent& event)
{
    event.Skip(event.GetSkipped());

    wxPoint MousePosPx = event.GetPosition();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float width = static_cast<float>(viewport[2]);
    float height = static_cast<float>(viewport[3]);

    glm::vec4 viewportvec(0.f, 0.f, width, height);
    glm::mat4 projection_matrix = graphic_engine->GetViewRef().GetProjectionMatrix();
    glm::vec3 mousePos = glm::unProject(vec3(MousePosPx.x, height - MousePosPx.y, 0.f), glm::mat4(1.f), projection_matrix, viewportvec);

    //wxString mousePosStr = std::string("x ") + std::to_string(mousePos.x) + std::string("  y ") + std::to_string(mousePos.y);
    //static_cast<wxFrame*>(GetParent())->SetStatusText(mousePosStr);

    if (!event.LeftIsDown())
    {
        if (switch_to_button_up)
        {
            translate_view += move_view;
            switch_to_button_up = false;
        }

        mouse_position_button_up = mousePos;
        move_view = vec3(0.f);
    }

    if (event.LeftIsDown())
    {
        move_view = mousePos - mouse_position_button_up;
        switch_to_button_up = true;
    }



    if (event.GetWheelRotation() != 0)
    {
        glm::vec4 preScaledMousePos = glm::inverse(glm::scale(scale_view)) * glm::inverse(glm::translate(translate_view)) * glm::vec4(mousePos, 1);

        int wheel_rotation = event.GetWheelRotation();

        scale += static_cast<float>(wheel_rotation) / 1200.f;
        auto factor = std::exp(scale);
        scale_view = vec3(factor, factor, 1.f);


        graphic_engine->GetViewRef().Scale(scale_view);

        glm::vec4 postScaleMousePos = glm::inverse(glm::translate(translate_view) * glm::scale(scale_view)) * glm::vec4(mousePos, 1);

        auto correction = postScaleMousePos - preScaledMousePos;
        auto back_calc = glm::scale(scale_view) * glm::translate(translate_view) * correction;

        translate_view += vec3(back_calc);

        //std::string statusText = std::string("zoom factor ") + std::to_string(factor * 100.f) + std::string("%");
        //static_cast<wxFrame*>(GetParent())->SetStatusText(statusText);
    }

    graphic_engine->GetViewRef().Translate(translate_view + move_view);

    parent->Refresh(false);
}

