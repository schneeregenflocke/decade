#version 460 core

uniform vec4 outline_color;
uniform vec4 fill_color;

out vec4 fragment_color;

void main()
{
    if (gl_PrimitiveID % 10 == 0 || gl_PrimitiveID % 10 == 1)
    {
        fragment_color = fill_color;
    } 
    else 
    {
        fragment_color = outline_color;
    }
}
