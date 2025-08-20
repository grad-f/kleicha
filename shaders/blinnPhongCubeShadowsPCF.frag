#version 450
#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_debug_printf : enable

struct GlobalData {
	vec4 ambientLight;
	mat4 bias;
	uint lightsCount;
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
	vec3 attenuationFactors;
	vec3 mPos;
	vec3 mvPos;
	mat4 viewProj;
	mat4 cubeViewProjs[6];
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
layout(set = 1, binding = 3) uniform sampler2D shadowSampler[];
layout(set = 1, binding = 4) uniform samplerCube cubeShadowSampler[];

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat int inTexID;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inVertView;
layout (location = 5) in vec3 inVertWorld;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
}pc;

float shadow_factor(uint samplerIndex, vec3 sampleDirection) {
	
	// sample depth of closest surface from the light's perspective at the sample direction
	float lightClosestDepth = texture(cubeShadowSampler[samplerIndex], sampleDirection).r;

	if (length(sampleDirection) < lightClosestDepth + 0.15f) {
		return 1.0f;
	}
	else
		return 0.5f;
}

/*float shadowLookup(vec4 shadowPos, float offsetX, float offsetY, uint mapIndex) {
	// determine whether the pixel fragment at the offset is in shadow (occluded)
	float notInShadow = textureProj(mapIndex, shadowPos + vec4(offsetX * 0.0005f * (1.0f - shadowPos.w), offsetY * 0.0011f * (1.0f - shadowPos.w), 0.005f, 0.0f));

	return notInShadow;
}*/

void main() {

	DrawData dd = draws[pc.drawId];
	
	// early return if rendering light
	if(dd.isLight != 0) {
		outColor = inColor;
		return;
	}

	vec3 N = normalize(inNormal);
	vec3 V = normalize(-inVertView);

	vec3 ambient = vec3(0.0f, 0.0f, 0.0f);	
	vec3 diffuse = vec3(0.0f, 0.0f, 0.0f); 
	vec3 specular = vec3(0.0f, 0.0f, 0.0f);
	vec3 lightContrib = vec3(0.0f, 0.0f, 0.0f);

	Material material = materials[dd.materialIndex];

	float attenuationFactor;
	float cosTheta;
	float cosPhi;
	Light light;
	float dist;
	vec3 L;
	vec3 H;

	for (uint i = 0; i < globals.lightsCount; ++i) {
		light = lights[i];
		L = light.mvPos - inVertView;
		// compute distance between vertex and light
		dist = sqrt(L.x * L.x + L.y * L.y + L.z * L.z);
		L = normalize(L);
		H = normalize(L - inVertView);

		// compute direction vector from light to fragment in world space to sample light cube map with
		vec3 fragmentToLight =  inVertWorld - light.mPos;

		float sFactor = shadow_factor(i, fragmentToLight);

		//if(i == 0)
			//debugPrintfEXT("%f | %f | %f\n", shadow_coord.x/shadow_coord.w, shadow_coord.y/shadow_coord.w, shadow_coord.z/shadow_coord.w);

		attenuationFactor = 1.0f / (light.attenuationFactors.x + light.attenuationFactors.y * dist + light.attenuationFactors.z * dist * dist);
		
		cosTheta = dot(N,L);

		// if the draw has a material
		if (dd.materialIndex > 0) {

			// compute ambient light contributions from global ambient and point light. 
			ambient = (globals.ambientLight * material.ambient + light.ambient * material.ambient).xyz;
			// diffuse is similar to ambient but the angle between the normal and light direction vectors is also a factor and per color channel
			diffuse = light.diffuse.xyz * material.diffuse.xyz * max(cosTheta, 0.0f);
			
			if (cosTheta > 0.0f) {
				// half-vector approximation of cos(phi) where phi is the angle between light reflect vector and view vector
				cosPhi = dot(H, N);
				specular = light.specular.xyz * material.specular.xyz * pow(max(cosPhi, 0.0f), material.shininess);
			}
		}
		else {
			ambient = (globals.ambientLight * light.ambient).xyz;
			diffuse = light.diffuse.xyz * max(cosTheta, 0.0f);
			if (cosTheta > 0.0f) {
				cosPhi = dot(H, N);
				specular = light.specular.xyz * pow(max(cosPhi, 0.0f), material.shininess*3.0f);
			}
		}

		lightContrib += attenuationFactor * ((sFactor * (diffuse + specular)) + ambient);
	}		

	if (dd.textureIndex > 0)
		outColor = texture(texSampler[dd.textureIndex], inUV) * vec4(lightContrib, 1.0f);
	else
		outColor = vec4(lightContrib, 1.0f);

}
