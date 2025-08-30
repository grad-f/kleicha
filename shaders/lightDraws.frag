#version 450
#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec4 inColor;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

void main() {
	outColor = inColor;
}
