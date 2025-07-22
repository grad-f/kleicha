#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

layout(scalar, binding = 0, set = 0) readonly buffer Vertices {
	Vertex vertices[];
};

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat int outTexID;

layout(scalar, push_constant) uniform constants {
	mat4 perspectiveProj;
	mat4 modelView;
	mat4 mvInvTr;
	int texID;
} PushConstants;

void main() {
	outTexID = PushConstants.texID;
	mat4 viewingTransform = PushConstants.perspectiveProj * PushConstants.modelView;

	gl_Position = viewingTransform * vec4(vertices[gl_VertexIndex].position, 1.0f);
	outUV = vertices[gl_VertexIndex].UV;
	outVertColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}