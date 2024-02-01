#version 450

//shader input
layout (location = 0) in vec2 inUV;
//output write
layout (location = 0) out vec4 outFragColor;

//texture to access
layout(set = 1, binding = 0) uniform sampler2D displayTexture;

void main() 
{
	// outFragColor = vec4(0.54, 0.0, 0.0, 1.0);
	outFragColor = texture(displayTexture,inUV);
}