#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : require

struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

struct DrawData {
	uint materialIndex;
	uint textureIndex;
	uint transformIndex;
};

struct GlobalData {
	vec4 ambientLight;
	mat4 bias;
	uint lightsCount;
};

struct Transform {
		mat4 model;
		mat4 modelView;
		mat4 modelViewInvTr;
};

layout(binding = 0, set = 0) readonly buffer Vertices {
	Vertex vertices[];
};

layout(binding = 1, set = 0) readonly buffer Draws {
	DrawData draws[];
};

layout(binding = 2, set = 0) readonly buffer Globals {
	GlobalData globals;
};

// view * model
layout(binding = 0, set = 1) readonly buffer Transforms {
	Transform transforms[];
};

layout(set = 0, binding = 3) uniform samplerCube texCubeSampler[];

layout (location = 0) in vec3 inSampleDir;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];

	outColor = texture(texCubeSampler[dd.textureIndex], inSampleDir);
}