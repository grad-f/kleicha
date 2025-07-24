#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable

struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

struct DrawData {
		uint materialIndex;
		uint vertexOffset;
};

layout(binding = 0, set = 0) readonly buffer Vertices {
	Vertex vertices[];
};

layout(binding = 1, set = 0) readonly buffer Draws {
	DrawData draws[];
};

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat uint outTexID;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	mat4 modelView;
	mat4 mvInvTr;
	uint drawId;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];
	mat4 viewingTransform = pc.perspectiveProj * pc.modelView;
	outTexID = dd.materialIndex;
	//debugPrintfEXT("%d\n", gl_VertexIndex + dd.vertexOffset);

	gl_Position = viewingTransform * vec4(vertices[gl_VertexIndex + dd.vertexOffset].position, 1.0f);
	outUV = vertices[gl_VertexIndex + dd.vertexOffset].UV;
	outVertColor = vec4(vertices[gl_VertexIndex + dd.vertexOffset].normal,1.0f);
}