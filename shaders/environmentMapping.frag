#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inFragWorld;

layout (location = 0) out vec4 outColor;

void main() {

	DrawData dd = draws[pc.drawId];

	// In world space, get direction vector from camera pos to vertex pos
	vec3 I = normalize(inFragWorld - pc.viewWorldPos);
	vec3 R = reflect(I, normalize(inNormal));
	R = vec3(R.x, -R.y, R.z);

	outColor = vec4(texture(texCubeSampler[6], R).rgb, 1.0f);

}
