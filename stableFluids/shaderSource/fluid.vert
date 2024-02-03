#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec2 outUV;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;


void main()
{
	gl_Position = vec4(vPosition, 1.0f);
  outUV = uv;
}