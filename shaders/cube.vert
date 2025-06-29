#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

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
} PushConstants;

void main() {
	gl_Position = PushConstants.perspectiveProj * vec4(PushConstants.pVertexBuffer.vertices[gl_VertexIndex].position, 1.0f);
}