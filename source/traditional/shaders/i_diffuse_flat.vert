#version 460 core

// Shader storage buffer with the models of each mesh
layout(std140, binding = 2) readonly buffer Models
{
  // When using [], then the size of this array is determined at the time the shader
  // is executed. The size is the rest of this buffer object range
	mat4[] models;
};

layout(location = 5) uniform mat4 view;
layout(location = 6) uniform mat4 projection;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
	int meshInstanceID = gl_InstanceID;

	mat4 model = models[meshInstanceID];

	fragPosition = vec3(mat4(1.0) * vec4(position, 1.0));
	fragNormal = normalize(mat3(mat4(1.0)) * normal);
	gl_Position = projection * view * model * vec4(position, 1.0);
}