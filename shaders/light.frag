#version 450
#include "common.h"

#define M_RCPPI 0.31830988618379067153776752674503f

#define M_PI 3.1415926535897932384626433832795f

layout (location = 0) in vec3 v3InPosition;
layout (location = 1) in vec3 v3InNormal;

layout (location = 0) out vec3 v3OutColor;

vec3 lightFalloff(vec3 v3LightIntensity, float fFalloff, vec3 v3LightPosition, vec3 v3Position) {
	float fDist = distance(v3LightPosition, v3Position);

	return v3LightIntensity / (fFalloff * fDist * fDist);
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

	// ambient
	v3RetColor += v3Diffuse * vec3(0.05f);

	return v3RetColor;
}

void main() {
	DrawData dd = draws[pc.uidrawId];
	Material md = materials[dd.uiMaterialIndex];
	
	// for now..
	PointLight light = lights[0];

	vec3 v3ViewDirection = normalize(globals.v3CameraPosition - v3InPosition);
	vec3 v3LightDirection = normalize(light.v3Position - v3InPosition);
	vec3 v3Normal = normalize(v3InNormal);

	// compute amount of light falling onto this point
	vec3 v3LightIrradiance = lightFalloff(light.v3Intensity, light.fFalloff, light.v3Position, v3InPosition);

	// compute shading
	vec3 v3LightColor = blinnPhong(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, md.v3Diffuse.xyz, md.v3Specular.xyz, md.fRoughness);

	v3OutColor = v3LightColor;
}