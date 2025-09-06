struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec4 tangent;
	vec3 bitangent;
};

struct GlobalData {
	vec4 ambientLight;
	mat4 bias;
	uint lightsCount;
	uint skyboxTexIndex;
};

struct DrawData {
	uint materialIndex;
	uint textureIndex;
	uint transformIndex;
};

struct Transform {
	mat4 model;
	mat4 modelView;
	mat4 modelInvTr;
	mat4 modelViewInvTr;
};

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
	float refractiveIndex;
};

struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	mat4 viewProj;
	mat4 cubeViewProjs[6];
	vec3 attenuationFactors;
	float lightSize;
	vec3 mPos;
	float frustumWidth;
	vec3 mvPos;
};

struct TextureData {
	uint albedoTexture;
	uint normalTexture;
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

layout(binding = 3, set = 0) readonly buffer Textures {
	TextureData textures[];
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

layout(set = 0, binding = 4) uniform sampler2D texSampler[];
layout(set = 0, binding = 4) uniform samplerCube texCubeSampler[];

layout(set = 1, binding = 3) uniform sampler2D shadowSampler[];
layout(set = 1, binding = 4) uniform samplerCube cubeShadowSampler[];

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	vec3 viewWorldPos;
	uint drawId;
	uint lightId;
}pc;