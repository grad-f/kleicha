#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout(location = 0) out float outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

layout (location = 0) in vec4 inFragWorldPos;
layout (location = 1) in flat uint inDrawId;

void main() {
	
	// compute distance in world space from fragment world pos to light
	vec3 lightToFragment = (inFragWorldPos.xyz - lights[pc.lightId].mPos);

	outColor = length(lightToFragment);
}
