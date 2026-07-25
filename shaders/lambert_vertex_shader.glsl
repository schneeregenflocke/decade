#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldnormal;
out vec3 worldposition;


void main()
{
  gl_Position = projection * view * model * vec4(position, 1.0);

  worldposition = vec3(model * vec4(position, 1.0));

  // Normalenmatrix statt model: bei nicht-uniformer Skalierung stünde die
  // Normale sonst schief zur Fläche. Sobald der 3D-Pfad läuft, gehört sie als
  // Uniform von der CPU — ein inverse() pro Vertex ist teuer.
  worldnormal = mat3(transpose(inverse(model))) * normal;
}
