#version 330 core

in vec3 worldnormal;
in vec3 worldposition;

uniform vec4 rawcolor;
uniform vec3 lightposition;

out vec4 color;

void main()
{
	// The interpolation across the triangle shortens the normal; without
	// normalize the dot() would be scaled wrongly. Exactly this computation per
	// fragment is what sets it apart from Gouraud shading.
	vec3 normal = normalize(worldnormal);
	vec3 lightdir = normalize(lightposition - worldposition);

	float diff = max(dot(normal, lightdir), 0.0);

	vec3 diffuse = diff * vec3(1.f, 1.f, 1.f);

	vec4 result = vec4(diffuse, 1.f) * rawcolor;

	color = result;
}
