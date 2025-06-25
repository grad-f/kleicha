#version 450

// output into the framebuffer associated with location 0
layout (location = 0) in vec3 inColor;
layout (location = 0) out vec4 outColor;
void main() {
	outColor = vec4(inColor, 1.0f);
}