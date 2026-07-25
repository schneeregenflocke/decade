#version 330 core

in vec3 worldnormal;
in vec3 worldposition;

uniform vec4 rawcolor;
uniform vec3 lightposition;

out vec4 color;

void main()
{
	// Die Interpolation über das Dreieck verkürzt die Normale; ohne normalize
	// wäre das dot() falsch skaliert. Genau diese Rechnung pro Fragment macht
	// den Unterschied zu Gouraud-Shading aus.
	vec3 normal = normalize(worldnormal);
	vec3 lightdir = normalize(lightposition - worldposition);

	float diff = max(dot(normal, lightdir), 0.0);

	vec3 diffuse = diff * vec3(1.f, 1.f, 1.f);

	vec4 result = vec4(diffuse, 1.f) * rawcolor;

	color = result;
}
