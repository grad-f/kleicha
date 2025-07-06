#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

layout (location = 0) out vec4 outVertColor;

struct Vertex {
	vec3 position;
};

// this effectively defines a type. a pointer to an array of vec3 positions. This is what we'll use to interpret the cube data provided using push constants and buffer device address
layout(scalar, buffer_reference) readonly buffer ptrBuffer {
	Vertex vertices[];
};

layout(scalar, push_constant) uniform constants {
	ptrBuffer pVertexBuffer;
	mat4 perspectiveProj;
	mat4 view;
	mat4 model;
	float timeFactor;
} pc;

mat4 buildRotateX(float phi);
mat4 buildRotateY(float phi);
mat4 buildRotateZ(float phi);
mat4 buildTranslate(float x, float y, float z);

void main() {
	// compute cube transformations to be applied to its local coordinate frame -- gl_InstanceIndex here provides a unique translation and rotation for each cube instance.
	float i = gl_InstanceIndex + pc.timeFactor;
	float a = sin(203.0 * i/8000.0) * 403.0;
	float b = sin(301.0 * i/4001.0) * 401.0;
	float c = sin(400.0 * i/6003.0) * 405.0;
	mat4 translate = buildTranslate(a, b, c);

	mat4 xRot = buildRotateX(i);
	mat4 yRot = buildRotateY(i);
	mat4 zRot = buildRotateZ(i);
	
	mat4 newModel = translate * xRot * yRot * zRot;

	ptrBuffer pVertBuffer = pc.pVertexBuffer;
	mat4 viewingTransform = pc.perspectiveProj * pc.view * newModel;

	gl_Position = viewingTransform * vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f);
	outVertColor = vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f) * 0.5f + vec4(0.5f, 0.5f, 0.5f, 0.5f);
}

mat4 buildRotateX(float phi) {
	return mat4(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, cos(phi), sin(phi), 0.0f,
	0.0f, -sin(phi), cos(phi), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f	);
}

mat4 buildRotateY(float phi) {
	return mat4(
	cos(phi), 0.0f, -sin(phi), 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	sin(phi), 0.0f, cos(phi), 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f	);
}

mat4 buildRotateZ(float phi) {
	return mat4(
	cos(phi), sin(phi), 0.0f, 0.0f,
	-sin(phi), cos(phi), 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
	);
}

mat4 buildTranslate(float x, float y, float z) {
	return mat4(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	x, y, z, 1.0f
	);
}