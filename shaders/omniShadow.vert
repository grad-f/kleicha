#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_multiview : enable
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

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float lightSize;
	vec3 attenuationFactors;
	float frustumWidth;
	vec3 mPos;
	vec3 mvPos;
	mat4 viewProj;
	mat4 cubeViewProjs[6];
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

layout(binding = 1, set = 1) readonly buffer Materials {
	Material materials[];
};

layout(binding = 2, set = 1) readonly buffer Lights {
	Light lights[];
};

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

layout(location = 0) out vec4 outVertWorld;
layout (location = 1) out flat uint outDrawId;

void main() {
	DrawData dd = draws[pc.drawId];

	// we choose to perform out lighting computations in camera-space.
	gl_Position = lights[pc.lightId].cubeViewProjs[gl_ViewIndex] * transforms[dd.transformIndex].model * vec4(vertices[gl_VertexIndex].position, 1.0f);
	outVertWorld = transforms[dd.transformIndex].model * vec4(vertices[gl_VertexIndex].position, 1.0f);

	//debugPrintfEXT("%f | %f | %f\n", lights[0].mvPos.x, lights[0].mvPos.y, lights[0].mvPos.z);
}