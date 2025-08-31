#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) in vec3 inSampleDir;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];

	outColor = texture(texCubeSampler[dd.textureIndex], inSampleDir);
}