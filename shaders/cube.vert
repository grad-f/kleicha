#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

// this effectively defines a type. a pointer to an array of vec3 positions. This is what we'll use to interpret the cube data provided using push constants and buffer device address
layout(scalar, buffer_reference) readonly buffer pVertexStruct {
	vec3 positions[];
};

layout(scalar, push_constant) uniform constants {
	pVertexStruct vertices;
} PushConstants;

void main() {
	gl_Position = vec4(PushConstants.vertices.positions[gl_VertexIndex], 1.0f);
}