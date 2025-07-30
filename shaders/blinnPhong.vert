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

struct GlobalData {
	vec4 ambientLight;
};

struct Transform {
		mat4 mv;
		mat4 mvInvTr;
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
	vec3 mPos;
	vec3 mvPos;
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

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat uint outTexID;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out vec3 outLightDir;
layout (location = 5) out vec3 outVertPos;
layout (location = 6) out vec3 outHalfVector;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];
	outTexID = dd.materialIndex;

	// we choose to perform out lighting computations in camera-space.
	outVertPos = (	transforms[pc.drawId].mv * vec4(vertices[gl_VertexIndex + dd.vertexOffset].position, 1.0f)	).xyz;
	outNormal = (transforms[pc.drawId].mvInvTr * vec4(vertices[gl_VertexIndex + dd.vertexOffset].normal, 1.0f)).xyz;
	outLightDir = lights[0].mvPos - outVertPos;
	outHalfVector = outLightDir - outVertPos;

	//debugPrintfEXT("%f | %f | %f\n", lights[0].mvPos.x, lights[0].mvPos.y, lights[0].mvPos.z);

	gl_Position = pc.perspectiveProj * transforms[pc.drawId].mv * vec4(vertices[gl_VertexIndex + dd.vertexOffset].position, 1.0f);
	outUV = vertices[gl_VertexIndex + dd.vertexOffset].UV;
}