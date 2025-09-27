#version 450
#extension GL_EXT_nonuniform_qualifier : require
#include "common.h"

#define M_RCPPI 0.31830988618379067153776752674503f

#define M_PI 3.1415926535897932384626433832795f

layout(constant_id = 0) const uint uiUseBlinnPhong = 0;
layout (location = 0) in vec3 v3InPosition;
layout (location = 1) in vec3 v3InNormal;
layout (location = 2) in vec2 v2InUV;

layout (location = 0) out vec3 v3OutColor;

vec3 lightFalloff(vec3 v3LightIntensity, vec3 v3Falloff, vec3 v3LightPosition, vec3 v3Position) {
	float fDist = distance(v3LightPosition, v3Position);

	return v3LightIntensity / (v3Falloff.x + v3Falloff.y * fDist + v3Falloff.z * fDist * fDist);
}

vec3 blinnPhong(vec3 v3Normal, vec3 v3LightDirection, vec3 v3ViewDirection, vec3 v3LightIrradiance, vec3 v3DiffuseColor, vec3 v3SpecularColor, float fRoughness) {

	vec3 v3HalfVector = normalize(v3ViewDirection + v3LightDirection);

	//vec3 v3Diffuse = max(dot(v3Normal, v3LightDirection), 0.0f) * v3DiffuseColor;
	vec3 v3Diffuse = v3DiffuseColor;
	vec3 v3Specular = pow(max(dot(v3Normal, v3HalfVector), 0.0f), fRoughness) * v3SpecularColor;

	v3Diffuse *= M_RCPPI;

	v3Specular *= (fRoughness + 8.0f) / (8.0f * M_PI);

	// normalize diffuse and specular contributions 
	vec3 v3RetColor = v3Diffuse + v3Specular;

	// Scale with respect to angle between geometry normal and direction to light
	v3RetColor *= max(dot(v3Normal, v3LightDirection), 0.0f);

	v3RetColor *= v3LightIrradiance;

	return v3RetColor;
}

// Trowbridge-Reitz Distribution Function
float TRDistribution(vec3 v3Normal, vec3 v3HalfVector, float fRoughness) {
	
	float fRsq = fRoughness * fRoughness;
	float fNdotH = max(dot(v3Normal, v3HalfVector), 0.0f);
	float fDenom = fNdotH * fNdotH * (fRsq - 1.0f) + 1.0f;

	return fRsq / (M_PI * fDenom * fDenom);
}

// approximates the ratio of light reflected to light refracted
vec3 schlickFresnel(vec3 v3LightDirection, vec3 v3Normal, vec3 v3SpecularColor) {
	float fLdotN = dot(v3LightDirection, v3Normal);

	return v3SpecularColor + (1.0f - v3SpecularColor) * pow(1.0f - fLdotN, 5.0f);
}

// GGX Geometry Function with normalizing cosine factor to simplify the overall equation
float GGXVisibility(vec3 v3Normal, vec3 v3LightDirection, vec3 v3ViewDirection, float fRoughness) {
	float fNdotL = max(dot(v3Normal, v3LightDirection), 0.0f);
	float fNdotV = max(dot(v3Normal, v3ViewDirection), 0.0f);
	float fRSq = fRoughness * fRoughness;
	float fRMod = 1.0f - fRSq;

	// Must consider both geometric obstruction and shadowing
	float fRecipG1 = fNdotL + sqrt(fRSq + (fRMod * fNdotL * fNdotL));
	float fRecipG2 = fNdotV + sqrt(fRSq + (fRMod * fNdotV * fNdotV));

	return 1.0f / (fRecipG1 * fRecipG2);
}

vec3 GGX(vec3 v3Normal, vec3 v3LightDirection, vec3 v3ViewDirection, vec3 v3LightIrradiance, vec3 v3DiffuseColor, vec3 v3SpecularColor, float fRoughness) {

	vec3 v3HalfVector = normalize(v3ViewDirection + v3LightDirection);

	vec3 v3Diffuse = v3DiffuseColor * M_RCPPI;

	// determines irradiance that is reflected rather than refracted
	vec3 v3F = schlickFresnel(v3LightDirection, v3HalfVector, v3SpecularColor);
	// determines the distribution (arrangement) of reflected light rays
	float fD = TRDistribution(v3Normal, v3HalfVector, fRoughness);
	// models self-shadowing component
	float fV = GGXVisibility(v3Normal, v3LightDirection, v3ViewDirection, fRoughness);

	// scale diffuse by ratio of light refracted
	v3Diffuse *= (1.0f - v3F);

	vec3 v3Color = v3Diffuse + (v3F * fD * fV);

	v3Color *= max(dot(v3Normal, v3LightDirection), 0.0f);

	v3Color *= v3LightIrradiance;

	return v3Color;
}

void main() {
	DrawData dd = draws[pc.uidrawId];
	Material md = materials[dd.uiMaterialIndex];
	
	vec3 v3ViewDirection = normalize(globals.v3CameraPosition - v3InPosition);
	vec3 v3Normal = normalize(v3InNormal);

	vec3 v3LightColor = vec3(0.0f);

	vec3 v3Diffuse = texture(texSampler[md.uiAlbedoTexture], v2InUV).rgb;
	float fRoughness = texture(texSampler[md.uiRoughnessTexture], v2InUV).g;

	for (uint i = 0; i < globals.uiNumPointLights; ++i) {
		PointLight light = lights[i];
		vec3 v3LightDirection = normalize(light.v3Position - v3InPosition);
		
		// compute amount of light falling onto this point
		vec3 v3LightIrradiance = lightFalloff(light.v3Intensity, light.v3Falloff, light.v3Position, v3InPosition);

		if (uiUseBlinnPhong > 0) {
			float fRoughnessPhong = (2.0f / (fRoughness * fRoughness)) - 2.0f;
			v3LightColor += blinnPhong(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3Diffuse, md.v3Specular.xyz, fRoughnessPhong);
		}
		else {
			// sample albedo
			v3LightColor += GGX(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, v3Diffuse, md.v3Specular.xyz, fRoughness);
		}
	}

	// add ambient contribution
	v3LightColor += v3Diffuse * 0.3f;
	v3OutColor = v3LightColor;
}