#version 450
#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormalView;
layout (location = 3) in vec3 inVertView;
layout (location = 4) in vec3 inTangentView;
layout (location = 5) in vec3 inBitangentView;


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

	for (int i = 0; i < globals.lightsCount; ++i) {
		light = lights[i];
		L = light.mvPos - inVertView;
		// compute distance between vertex and light
		dist = sqrt(L.x * L.x + L.y * L.y + L.z * L.z);
		L = normalize(L);
		H = normalize(L - inVertView);

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

		lightContrib += attenuationFactor * (ambient + diffuse + specular);
	}		

	if (textureData.albedoTexture > 0)
		outColor = texture(texSampler[textureData.albedoTexture], inUV) * vec4(lightContrib, 1.0f);
	else
		outColor = vec4(lightContrib, 1.0f);

}