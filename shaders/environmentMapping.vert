#version 450
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outVertWorld;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
	uint lightId;
	vec3 viewWorldPos;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];

	// vertex pos vector with respect to view
	outVertWorld = (transform.model * vec4(vert.position, 1.0f)).xyz;
	outNormal =  mat3(transpose(inverse(transform.model))) * vert.normal;

	gl_Position = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);
}