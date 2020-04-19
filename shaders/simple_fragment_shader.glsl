#version 330 core


//uniform vec4 rawcolor;


in vec4 vertexcolor;


out vec4 color;


void main()
{
	color = vertexcolor;
}
