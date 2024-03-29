#version 460

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

out vec2 fragUV;

void main()
{
  fragUV = uv;
  gl_Position = vec4(position, 0.0, 1.0);
}