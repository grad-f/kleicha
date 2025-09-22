struct Vertex {
	vec3 v3Position;
	vec2 v2UV;
	vec3 v3Normal;
	vec4 v4Tangent;
	vec3 v4Bitangent;
};

struct GlobalData {
	vec3 v3CameraPosition;
	uint uilightCount;
};

struct DrawData {
	uint uiMaterialIndex;
	uint uiTransformIndex;
};

struct Transform {
	mat4 m4Model;
	mat4 m4ModelInvTr;
};

struct Material {
	vec3 v3Diffuse;	
	vec3 v3Specular;
	float fShininess;
	uint uiAlbedoTexture;
	uint uiNormalTexture;
	uint uiHeightTexture;
	uint uiEmissiveTexture;
};

struct PointLight {
	vec3 v3Position;
	vec3 v3Intensity;
	float m_fFalloff;
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
	PointLight lights[];
};

layout(set = 0, binding = 3) uniform sampler2D texSampler[];
layout(set = 0, binding = 3) uniform samplerCube texCubeSampler[];

layout(set = 1, binding = 3) uniform sampler2D shadowSampler[];
layout(set = 1, binding = 4) uniform samplerCube cubeShadowSampler[];

layout(push_constant) uniform constants {
	// orthographic projection * perspective * view
	mat4 m4ViewProjection;
	uint uidrawId;
}pc;