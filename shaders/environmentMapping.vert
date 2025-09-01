#version 450
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outVertWorld;
layout (location = 2) out vec2 outUV;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];
	
	// vertex pos vector with respect to view
	outVertWorld = (transform.model * vec4(vert.position, 1.0f)).xyz;
	outNormal =  (transform.modelInvTr * vec4(vert.normal, 1.0f)).xyz;
	outUV = vert.UV;

	gl_Position = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);
}