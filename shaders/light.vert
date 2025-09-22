#version 450
#include "common.h"

layout (location = 0) out vec3 v3OutColor;

vec3 lightFalloff(vec3 v3LightIntensity, float fFalloff, vec3 v3LightPosition, vec3 v3Position) {
	float fDist = distance(v3LightPosition, v3Position);

	return v3LightIntensity / (fFalloff * fDist * fDist);
}

vec3 blinnPhong(vec3 v3Normal, vec3 v3LightDirection, vec3 v3ViewDirection, vec3 v3LightIrradiance, vec3 v3DiffuseColor, vec3 v3SpecularColor, float fShininess) {

	vec3 v3HalfVector = normalize(v3ViewDirection + v3LightDirection);

	vec3 v3Specular = pow(max(dot(v3Normal, v3HalfVector), 0.0f), fShininess) * v3SpecularColor;

	vec3 v3RetColor = v3DiffuseColor + v3Specular;

	// Scale with respect to angle between geometry normal and direction to light
	v3RetColor *= max(dot(v3Normal, v3LightDirection), 0.0f);

	v3RetColor *= v3LightIrradiance;

	return v3RetColor;
}

void main() {
	DrawData dd = draws[pc.uidrawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform td = transforms[dd.uiTransformIndex];
	Material md = materials[dd.uiMaterialIndex];
	
	// for now..
	PointLight light = lights[0];

	vec4 v4Position = td.m4Model * vec4(vert.v3Position, 1.0f);
	gl_Position = pc.m4ViewProjection * v4Position;

	vec3 v3Normal = normalize((td.m4ModelInvTr * vec4(vert.v3Normal, 1.0f)).xyz);
	vec3 v3ViewDirection = normalize(globals.v3CameraPosition - v4Position.xyz);
	vec3 v3LightDirection = normalize(light.v3Position - v4Position.xyz);

	// compute amount of light falling onto this point
	vec3 v3LightIrradiance = lightFalloff(light.v3Intensity, 0.01f, light.v3Position, v4Position.xyz);

	// compute shading
	vec3 v3LightColor = blinnPhong(v3Normal, v3LightDirection, v3ViewDirection, v3LightIrradiance, md.v3Diffuse.xyz, md.v3Specular.xyz, md.fShininess);

	v3OutColor = v3LightColor;
}