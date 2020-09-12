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

std::ostream& operator<<(std::ostream& out, const glm::dvec4& value)
{
    out << "x: " << value.x << " y: " << value.y << " z: " << value.z << " w: " << value.w;
    return out;
}


GLCanvas::GLCanvas(wxWindow* parent, const wxGLAttributes& canvas_attributes) : 
    wxGLCanvas(parent, canvas_attributes),
    openGL_ready(0)
{}

GraphicEngine* GLCanvas::GetGraphicEngine()
{
    return graphic_engine.get();
}

void GLCanvas::LoadOpenGL(const std::array<int, 2>& version)
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

std::string GLCanvas::GetGLVersionString()
{
    std::string version_string;
    version_string += "GL_VERSION " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VERSION))) + '\n';
    version_string += "GL_VENDOR " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_VENDOR))) + '\n';
    version_string += "GL_RENDERER " + wxString::FromUTF8(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    return version_string;
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
    mouse_position_button_up = glm::vec3(0.f);
}

void MouseInteraction::OnMouse(wxMouseEvent& event)
{
    //event.Skip();

    wxPoint MousePosPx = event.GetPosition();
    double d_mouse_pos_x = static_cast<double>(MousePosPx.x);
    double d_mouse_pos_y = static_cast<double>(MousePosPx.y);

    std::array<GLint, 4> viewport;
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    const double width = static_cast<double>(viewport[2]);
    const double height = static_cast<double>(viewport[3]);

    glm::dvec4 viewportvec(0.0, 0.0, width, height);
    glm::dmat4 projection_matrix = graphic_engine->GetViewRef().GetProjectionMatrix();
    glm::dvec3 mousePos = glm::unProject(glm::dvec3(d_mouse_pos_x, height - d_mouse_pos_y, 0.0), glm::dmat4(1.0), projection_matrix, viewportvec);

    if (!event.LeftIsDown())
    {
        if (switch_to_button_up)
        {
            translate_view += move_view;
            switch_to_button_up = false;
        }

        mouse_position_button_up = mousePos;
        move_view = glm::dvec3(0.0);
    }

    if (event.LeftIsDown())
    {
        move_view = mousePos - glm::dvec3(mouse_position_button_up);
        switch_to_button_up = true;
    }

    if (event.GetWheelRotation() != 0)
    {
        glm::dvec4 preScaledMousePos =
            glm::inverse(glm::scale(glm::dvec3(scale_view))) *
            glm::inverse(glm::translate(glm::dvec3(translate_view))) *
            glm::dvec4(mousePos, 1);


        int wheel_rotation = event.GetWheelRotation();

        const double mouse_wheel_step = 1200.0;
        scale += static_cast<double>(wheel_rotation) / mouse_wheel_step;
        const double factor(std::exp(scale));
        scale_view = glm::dvec3(factor, factor, 1.0);
        //std::wcout << "zoom factor " << factor << "\n";

        graphic_engine->GetViewRef().Scale(scale_view);

        glm::dvec4 postScaleMousePos =
            glm::inverse(glm::translate(translate_view) *
            glm::scale(scale_view)) *
            glm::dvec4(mousePos, 1);

        

        glm::dvec4 correction = postScaleMousePos - preScaledMousePos;


        glm::dvec4 back_calc = glm::scale(scale_view) * glm::translate(translate_view) * correction;

        translate_view += glm::dvec3(back_calc);
    }

    graphic_engine->GetViewRef().Translate(translate_view + move_view);

    parent->Refresh(false);
}

/*constexpr wxGLContextAttrs GLCanvas::defaultAttrs()
{
    /home/titan99/code/decade/src/gui/gl_canvas.cpp: In Konstruktor »GLCanvas::GLCanvas(wxWindow*, const wxGLAttributes&)«:
    /home/titan99/code/decade/src/gui/gl_canvas.cpp:24:42: Fehler: Adresse eines rvalues wird ermittelt [-fpermissive]
    context(this, nullptr, &defaultAttrs()),
    context_attributes.PlatformDefaults().CoreProfile().OGLVersion(3, 2).EndList();
    return context_attributes;
}*/
