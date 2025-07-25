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
} PushConstants;

void main() {
	
	ptrBuffer pVertBuffer = PushConstants.pVertexBuffer;
	mat4 viewingTransform = PushConstants.perspectiveProj * PushConstants.view * PushConstants.model;
	
	gl_Position = viewingTransform * vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f);
	outVertColor = vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f) * 0.5f + vec4(0.5f, 0.5f, 0.5f, 0.5f);
}