#version 450
#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormalView;
layout (location = 3) in vec3 inVertView;
layout (location = 4) in vec3 inVertWorld;
layout (location = 5) in vec3 inTangentView;
layout (location = 6) in vec3 inBitangentView;

layout (location = 0) out vec4 outColor;

// We will be performing our lighting computations in tangent space in the future. This is temporary.
vec3 calcShadingNormal(uint textureIndex) {
	// normal
	vec3 N = normalize(inNormalView);
	// tangent
	vec3 T = normalize(inTangentView);
	// bitangent
	vec3 B = normalize(inBitangentView);
	
	// perspective interpolation results in normal and tangent potentially not orthogonal. Therefore, re-orthogonalize using Gram-Schmidt process
	T = normalize(T - dot(T, N) * N);
	B = normalize(B - dot(B, N) * N - dot(B, T) * T); 

	// form change of coordinates (tangent to view) transformation
	mat3 TBN = mat3(T,B,N);
	
	// sample normal map
	vec3 shadingNormalTBN = texture(texSampler[textureIndex], inUV).rgb;

	// map [0,1] to [-1,1]
	shadingNormalTBN = shadingNormalTBN * 2.0f - 1.0f;

	vec3 shadingNormalView = TBN * shadingNormalTBN;
	shadingNormalView = normalize(shadingNormalView);

	return shadingNormalView;
}

float textureProj(uint samplerIndex, vec4 shadowCoord) {
	
	// homogenize shadow coordinates
	shadowCoord /= shadowCoord.w;

	float closestDepth = texture(shadowSampler[samplerIndex], shadowCoord.xy).r;

	// if shadow map depth is greater than pixel fragment depth, the pixel fragment is in the shadow.
	float notInShadow = closestDepth > shadowCoord.z ? 0.0f : 1.0f;

	return notInShadow;
}

float shadowLookup(vec4 shadowPos, float offsetX, float offsetY, uint mapIndex) {
	// determine whether the pixel fragment at the offset is in shadow (occluded)
	float notInShadow = textureProj(mapIndex, shadowPos + vec4(offsetX * 0.00125f * (1.0f - shadowPos.w), offsetY * 0.00222f * (1.0f - shadowPos.w), 0.005f, 0.0f));

	return notInShadow;
}

void main() {
	DrawData dd = draws[pc.drawId];
	Material material = materials[dd.materialIndex];
	TextureData textureData = textures[dd.textureIndex];

	vec4 albedoSample;
	if (textureData.albedoTexture > 0) {
		albedoSample = texture(texSampler[textureData.albedoTexture], inUV);
		
		if (albedoSample.a < 0.5f)
			discard;
	}

	vec3 N;

	if(textureData.normalTexture > 0 && pc.enableBumpMapping > 0)
		N = calcShadingNormal(textureData.normalTexture);
	else
		N = normalize(inNormalView);

	vec3 V = normalize(-inVertView);

	vec3 ambient = vec3(0.0f, 0.0f, 0.0f);	
	vec3 diffuse = vec3(0.0f, 0.0f, 0.0f); 
	vec3 specular = vec3(0.0f, 0.0f, 0.0f);
	vec3 lightContrib = vec3(0.0f, 0.0f, 0.0f);

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

		// determine if this pixel fragment is occluded with respect to the this light (in shadow)
		// applies light transformation to pixel fragment world pos then the bias to map NDC to [0,1] (technically not [0,1] because the shadow_coord has yet to be homogenized)
		vec4 shadow_coord = globals.bias * light.viewProj * vec4(inVertWorld,1.0f);

		//if(i == 0)
			//debugPrintfEXT("%f | %f | %f\n", shadow_coord.x/shadow_coord.w, shadow_coord.y/shadow_coord.w, shadow_coord.z/shadow_coord.w);

		attenuationFactor = 1.0f / (light.attenuationFactors.x + light.attenuationFactors.y * dist + light.attenuationFactors.z * dist * dist);

		// s here is for shadow
		float sOffsetFactor = 2.5f;
		
		// compute 1 of 4 sample patterns for the given pixel fragment (0,0), (1,0), (0,1) or (1,1) and scale this by our offset factor
		vec2 offset = mod(floor(gl_FragCoord.xy), 2.0f) * sOffsetFactor;

		// Using the offsets, sample close-by pixel fragments are not in shadow
		/*float sFactor = shadowLookup(shadow_coord, -1.5f * sOffsetFactor + offset.x, 1.5f * sOffsetFactor - offset.y, i);
		sFactor +=		shadowLookup(shadow_coord, -1.5f * sOffsetFactor + offset.x, -0.5f * sOffsetFactor - offset.y, i);
		sFactor +=		shadowLookup(shadow_coord, 0.5f * sOffsetFactor + offset.x, 1.5f * sOffsetFactor - offset.y, i);
		sFactor +=		shadowLookup(shadow_coord, 0.5f * sOffsetFactor + offset.x, -0.5f * sOffsetFactor - offset.y, i);

		// average samples
		sFactor /= 4.0f;*/

		float endp = sOffsetFactor * 3.0f + sOffsetFactor / 2.0f;
		float sFactor = 0.0f;
		for (float m = -endp; m <= endp; m+=sOffsetFactor) {
			for (float n = -endp; n <= endp; n+=sOffsetFactor) {
				sFactor += shadowLookup(shadow_coord, m, n, i);
			}
		}

		sFactor /= 64.0f;

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

	if (textureData.albedoTexture > 0)
		outColor = albedoSample * vec4(lightContrib, 1.0f);
	else
		outColor = vec4(lightContrib, 1.0f);

}
