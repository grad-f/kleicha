#version 450

// output into the framebuffer associated with location 0
layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}