#version 450

layout(location = 0) out vec3 outColor;

	// NDC [-1,1] x [-1,1] x [0,1]
	vec3 positions[3] = vec3[] (
		vec3(-0.5f, 0.75f, 0.0f),
		vec3(0.5f, 0.75f, 0.0f),
		vec3(0.0f, -0.75f, 0.0f)
	);

	vec3 colors[3] = vec3[] (
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f)
	);

void main() {
	gl_Position = vec4(positions[gl_VertexIndex].xyz, 1.0f);
	outColor = colors[gl_VertexIndex];
}