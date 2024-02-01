#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform  CameraBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

vec3 hsvToRgb(float h, float s, float v) {
    float r = abs(h * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(h * 6.0 - 2.0);
    float b = 2.0 - abs(h * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0) * v;
}

void main() {

    gl_PointSize = 1.0;
    gl_Position = cameraData.viewproj * vec4(inPosition.xyz, 1.0);
    float distance = distance(gl_Position.xy, vec2(0.0f));
    float normalizedDistance = clamp(distance / sqrt(12.0), 0.0, 1.0); // Normalize distance

    float hue = normalizedDistance;
    float value = mix(1.0, 0.1, normalizedDistance);
    fragColor = vec4(hsvToRgb(hue, 1.0, value),1.0);
}