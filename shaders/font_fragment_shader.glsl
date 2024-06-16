#version 330 core

in vec2 vertex_texture_coord;

uniform sampler2D texture_sampler;
uniform vec4 texture_color;

out vec4 color;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture_sampler, vertex_texture_coord).r);
	color = texture_color * sampled;
}  
