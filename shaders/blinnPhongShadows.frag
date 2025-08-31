#version 450
#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat int inTexID;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inVertView;
layout (location = 5) in vec3 inVertWorld;
layout (location = 6) in flat uint inDrawId;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

float textureProj(uint samplerIndex, vec4 shadowCoord) {
	
	// homogenize shadow coordinates
	shadowCoord /= shadowCoord.w;

	float closestDepth = texture(shadowSampler[samplerIndex], shadowCoord.xy).r;

	// if shadow map depth is greater than pixel fragment depth, the pixel fragment is in the shadow.
	float notInShadow = closestDepth > shadowCoord.z ? 0.0f : 1.0f;

	return notInShadow;
}

void main() {

	DrawData dd = draws[pc.drawId];

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

	for (int i = 0; i < globals.lightsCount; ++i) {
		light = lights[i];
		L = light.mvPos - inVertView;
		// compute distance between vertex and light
		dist = sqrt(L.x * L.x + L.y * L.y + L.z * L.z);
		L = normalize(L);
		H = normalize(L - inVertView);

		// determine if this pixel fragment is occluded with respect to the this light (in shadow)
		// applies light transformation to pixel fragment world pos then the bias to map NDC to [0,1] (technically not [0,1] because the shadow_coord has yet to be homogenized)
		vec4 shadow_coord = globals.bias * light.viewProj * vec4(inVertWorld,1.0f);

		// textureProj homogenizes shadow_coord and uses the resulting vec3 to compare the depth of this pixel fragment and that of which is stored in the shadow map.
		// returns 1.0f if pixel fragment's depth is greater (closer in our case) than that of what is stored.
		float notInShadow = textureProj(i, shadow_coord);

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

		lightContrib += attenuationFactor * ambient;

		if (notInShadow == 1.0f)
			lightContrib += attenuationFactor * (diffuse + specular);

		/*if (notInShadow == 1.0f)
			lightContrib = vec3(1.0f, 0.0f, 0.0f);*/
	}		

	if (dd.textureIndex > 0)
		outColor = texture(texSampler[dd.textureIndex], inUV) * vec4(lightContrib, 1.0f);
	else
		outColor = vec4(lightContrib, 1.0f);

}
