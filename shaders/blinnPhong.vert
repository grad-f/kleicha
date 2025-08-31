#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat uint outTexID;
layout (location = 3) out vec3 outNormal;
layout (location = 4) out vec3 outVertView;
layout (location = 5) out flat uint outDrawId;

void main() {
	DrawData dd = draws[pc.drawId];
	outTexID = dd.materialIndex;
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];

	// we choose to perform out lighting computations in camera-space.

	// vertex in camera space
	outVertView = (	transform.modelView * vec4(vert.position, 1.0f)	).xyz;

	outNormal = (	transform.modelViewInvTr * vec4(vert.normal, 1.0f)	).xyz;

	gl_Position = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);
	outUV = vert.UV;
}