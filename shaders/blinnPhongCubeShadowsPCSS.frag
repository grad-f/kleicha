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

vec3 offsetDirections[128] = vec3[](

    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),


    vec3( 2,  0,  0), vec3(-2,  0,  0), vec3( 0,  2,  0), vec3( 0, -2,  0),
    vec3( 0,  0,  2), vec3( 0,  0, -2), vec3( 2,  2,  0), vec3(-2, -2,  0),
    vec3( 2,  0,  2), vec3(-2,  0, -2), vec3( 0,  2,  2), vec3( 0, -2, -2),
    vec3( 2, -2,  0), vec3(-2,  2,  0), vec3( 2,  0, -2), vec3(-2,  0,  2),
    vec3( 0,  2, -2), vec3( 0, -2,  2), vec3( 1,  2,  1), vec3(-1, -2, -1),
    vec3( 2,  1,  1), vec3(-2, -1, -1), vec3( 1,  1,  2), vec3(-1, -1, -2),
    vec3( 2, -1,  1), vec3(-2,  1, -1), vec3( 1, -2,  1), vec3(-1,  2, -1),
    vec3( 1,  1, -2), vec3(-1, -1,  2), vec3( 2,  1, -1), vec3(-2, -1,  1),
    vec3( 1, -1,  2), vec3(-1,  1, -2), vec3( 2, -1, -1), vec3(-2,  1,  1),
    vec3( 1, -2, -1), vec3(-1,  2,  1), vec3( 1,  2, -1), vec3(-1, -2,  1),
    vec3( 2,  2,  2), vec3(-2, -2, -2), vec3( 2, -2,  2), vec3(-2,  2, -2),

    vec3( 3,  0,  0), vec3(-3,  0,  0), vec3( 0,  3,  0), vec3( 0, -3,  0),
    vec3( 0,  0,  3), vec3( 0,  0, -3), vec3( 3,  3,  0), vec3(-3, -3,  0),
    vec3( 3,  0,  3), vec3(-3,  0, -3), vec3( 0,  3,  3), vec3( 0, -3, -3),
    vec3( 3, -3,  0), vec3(-3,  3,  0), vec3( 3,  0, -3), vec3(-3,  0,  3),
    vec3( 0,  3, -3), vec3( 0, -3,  3), vec3( 1,  3,  1), vec3(-1, -3, -1),
    vec3( 3,  1,  1), vec3(-3, -1, -1), vec3( 1,  1,  3), vec3(-1, -1, -3),
    vec3( 3, -1,  1), vec3(-3,  1, -1), vec3( 1, -3,  1), vec3(-1,  3, -1),
    vec3( 1,  1, -3), vec3(-1, -1,  3), vec3( 3,  1, -1), vec3(-3, -1,  1),
    vec3( 1, -1,  3), vec3(-1,  1, -3), vec3( 3, -1, -1), vec3(-3,  1,  1),
    vec3( 1, -3, -1), vec3(-1,  3,  1), vec3( 1,  3, -1), vec3(-1, -3,  1),
    vec3( 3,  3,  3), vec3(-3, -3, -3), vec3( 3, -3,  3), vec3(-3,  3, -3),
    vec3( 2,  3,  1), vec3(-2, -3, -1), vec3( 3,  2,  1), vec3(-3, -2, -1),
    vec3( 1,  2,  3), vec3(-1, -2, -3), vec3( 3,  1,  2), vec3(-3, -1, -2),
    vec3( 2, -3,  1), vec3(-2,  3, -1), vec3( 3, -2,  1), vec3(-3,  2, -1),
    vec3( 1, -2,  3), vec3(-1,  2, -3), vec3( 3, -1,  2), vec3(-3,  1, -2),
    vec3( 2,  2, -3), vec3(-2, -2,  3), vec3( 3,  2, -2), vec3(-3, -2,  2)
);

#define NEAR_PLANE 0.1f
#define LIGHT_WORLD_SIZE 12.5f
#define LIGHT_FRUSTUM_WIDTH 3.75f
#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)
#define PCF_SAMPLES 64
#define BLOCKER_SAMPLES 25

void findBlocker(uint sCubeMapIndex, out float avgBlockerDepth, out float numBlockers, vec3 wFragToLight, float receiverDist, float mappedReceiverDist, float lightSize) {

		float searchWidth = lightSize * (mappedReceiverDist - NEAR_PLANE) / mappedReceiverDist;

		float blockerSum = 0.0f;
		numBlockers = 0;

		// sample cube shadow map and accumulate depths
		for (int i = 0; i < BLOCKER_SAMPLES; ++i) {
			float depthSample = texture(cubeShadowSampler[sCubeMapIndex], wFragToLight + offsetDirections[i] * searchWidth).r;

			// is blocker. we also check that the depth sample isn't geometry far away in the background that will skew the average blocker depth
			if (depthSample + 0.1f < receiverDist) {

			/*if(sCubeMapIndex == 0)
				debugPrintfEXT("%f\n", depthSample);*/

				blockerSum += depthSample;
				numBlockers++;
			}
		}

		avgBlockerDepth = (blockerSum * 0.001f) / numBlockers;

}

float computeShadow(uint sCubeMapIndex, vec3 wFragToLight, float lightSize) {
	
	// magnitude of pixel frag to light vector
	float receiverDist = length(wFragToLight);
	float mappedReceiverDist = receiverDist * 0.001f;

	// determine the average depth of blockers in the vicinity of this pixel fragment
	float avgBlockerDepth = 0.0f;
	float numBlockers = 0.0f;

	findBlocker(sCubeMapIndex, avgBlockerDepth, numBlockers, wFragToLight, receiverDist, mappedReceiverDist, lightSize);

	// not in shadow
	if (numBlockers < 1.0f ) {
		return 1.0f;	
	}
	
	float penumbraRatio = (mappedReceiverDist - avgBlockerDepth) / avgBlockerDepth;
	float filterRadius = penumbraRatio * lightSize * NEAR_PLANE / receiverDist;

	float sFactor = 0.0f;

	for (int i = 0; i < PCF_SAMPLES; ++i) {
		float depthSample = texture(cubeShadowSampler[sCubeMapIndex], wFragToLight + offsetDirections[i] * filterRadius).r;
		
		if (depthSample + 0.1f > receiverDist) {
			sFactor += 1.0f;
		}

	}

	sFactor /= float(PCF_SAMPLES); 

	return sFactor;
}

void main() {

	DrawData dd = draws[pc.drawId];
	Material material = materials[dd.materialIndex];
	TextureData textureData = textures[dd.textureIndex];

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

		// compute direction vector from light to fragment in world space to sample light cube map with
		vec3 fragmentToLight =  inVertWorld - light.mPos;
		float lightSize = light.lightSize / light.frustumWidth;

		float sFactor = computeShadow(i, fragmentToLight, lightSize);

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
		lightContrib += (sFactor * (attenuationFactor * (diffuse + specular)) + attenuationFactor * ambient);
	}		

	if (textureData.albedoTexture > 0)
		outColor = texture(texSampler[textureData.albedoTexture], inUV) * vec4(lightContrib, 1.0f);
	else
		outColor = vec4(lightContrib, 1.0f);

}
