#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct GlobalData {
	vec4 ambientLight;
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

layout(binding = 2, set = 0) readonly buffer Globals {
	GlobalData globals;
};

layout(binding = 1, set = 1) readonly buffer Materials {
	Material materials[];
};

layout(binding = 2, set = 1) readonly buffer Lights {
	Light lights[];
};

layout(set = 0, binding = 3) uniform sampler2D texSampler[];

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat int inTexID;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inLightDir;
layout (location = 5) in vec3 inVertPos;
layout (location = 6) in vec3 inHalfVector;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
}pc;

void main() {
    //outColor = inColor;
    //outColor = inColor * texture(texSampler[inTexID], inUV);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightDir);
	vec3 V = normalize(-inVertPos);
	vec3 H = normalize(inHalfVector);

	// half-vector approximation of cos(phi) where phi is the angle between light reflect vector and view vector
	float cosPhi = dot(H, N);

	// compute ambient light contributions from global ambient and point light. 
	vec3 ambient = (globals.ambientLight * materials[pc.drawId].ambient + lights[0].ambient * materials[pc.drawId].ambient).xyz;

	// diffuse is similar to ambient but the angle between the normal and light direction vectors is also a factor and per color channel
	vec3 diffuse = lights[0].diffuse.xyz * materials[pc.drawId].diffuse.xyz * max(dot(N, L), 0.0f);
	
	vec3 specular = lights[0].specular.xyz * materials[pc.drawId].specular.xyz * pow(max(cosPhi, 0.0f), materials[pc.drawId].shininess);

	outColor = vec4(ambient + diffuse + specular, 1.0f);
}
