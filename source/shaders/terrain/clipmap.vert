#version 460 core

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;

layout(location = 3) uniform float levelScale;
layout(location = 4) uniform vec2 offset;

// Inputs
layout(location = 0) in vec3 position;

void main()
{
  vec2 xz = offset + (model * vec4(position, 1.0)).xz * levelScale;

  float y = 0.0;

  vec3 worldPosition = vec3(y);
  worldPosition.xz = xz;

	gl_Position = projection * view * vec4(worldPosition, 1.0);
}