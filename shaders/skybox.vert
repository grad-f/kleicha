#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_ARB_shader_draw_parameters : enable

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

layout (location = 0) out vec3 outSampleDir;


layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];

	outSampleDir = vec3(vert.position.x, -vert.position.y, vert.position.z);

	vec4 pos = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);
	gl_Position = vec4(pos.xy, 0.0f, pos.w);
}