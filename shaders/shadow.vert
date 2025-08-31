#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

layout (location = 0) out flat uint outDrawId;

void main() {
	DrawData dd = draws[pc.drawId];

	// we choose to perform out lighting computations in camera-space.
	gl_Position = lights[pc.lightId].viewProj * transforms[dd.transformIndex].model * vec4(vertices[gl_VertexIndex].position, 1.0f);

	//debugPrintfEXT("%f | %f | %f\n", lights[0].mvPos.x, lights[0].mvPos.y, lights[0].mvPos.z);
}