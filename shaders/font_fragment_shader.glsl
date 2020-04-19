#version 330 core

in vec2 vertexTextureCoord;

out vec4 color;

uniform sampler2D textureSampler;

uniform vec4 texColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(textureSampler, vertexTextureCoord).r);
	color = texColor * sampled;
	//gl_FragColor = texColor * sampled;
}  
