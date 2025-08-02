#version 450
#extension GL_EXT_nonuniform_qualifier : require

struct GlobalData {
	vec4 ambientLight;
};

struct DrawData {
	uint materialIndex;
	uint textureIndex;
	uint transformIndex;
	uint isLight;
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

layout(binding = 1, set = 0) readonly buffer Draws {
	DrawData draws[];
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
	DrawData dd = draws[pc.drawId];

	if (dd.isLight == 0) {
		vec3 N = normalize(inNormal);
		vec3 L = normalize(inLightDir);
		vec3 V = normalize(-inVertPos);
		vec3 H = normalize(inHalfVector);

		vec3 ambient = vec3(0.0f, 0.0f, 0.0f);
		vec3 diffuse = vec3(0.0f, 0.0f, 0.0f); 
		vec3 specular = vec3(0.0f, 0.0f, 0.0f);
	
		float cosTheta = dot(N,L);

		if (dd.materialIndex > 0) {

			// compute ambient light contributions from global ambient and point light. 
			ambient = (globals.ambientLight * materials[dd.materialIndex].ambient + lights[0].ambient * materials[dd.materialIndex].ambient).xyz;
			// diffuse is similar to ambient but the angle between the normal and light direction vectors is also a factor and per color channel
			diffuse = lights[0].diffuse.xyz * materials[dd.materialIndex].diffuse.xyz * max(cosTheta, 0.0f);
		
			specular = vec3(0.0f, 0.0f, 0.0f);
			if (cosTheta > 0.0f) {
				// half-vector approximation of cos(phi) where phi is the angle between light reflect vector and view vector
				float cosPhi = dot(H, N);
				specular = lights[0].specular.xyz * materials[dd.materialIndex].specular.xyz * pow(max(cosPhi, 0.0f), materials[dd.materialIndex].shininess);
			}
		}
		else {
	
			ambient = (globals.ambientLight * lights[0].ambient).xyz;
			diffuse = lights[0].diffuse.xyz * max(cosTheta, 0.0f);
			if (cosTheta > 0.0f) {
				float cosPhi = dot(H, N);
				specular = lights[0].specular.xyz * pow(max(cosPhi, 0.0f), materials[dd.materialIndex].shininess*3.0f);
			}
		}

		if (dd.textureIndex > 0)
			outColor = texture(texSampler[dd.textureIndex], inUV) * vec4(ambient + diffuse + specular, 1.0f);
		else
			outColor = vec4(ambient + diffuse + specular, 1.0f);
	} else {
		outColor = inColor;
	}
}
