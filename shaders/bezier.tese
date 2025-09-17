#version 450

layout(quads, equal_spacing, ccw) in;

void main() {
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	gl_Position = vec4(u, 0, v, 1);
}