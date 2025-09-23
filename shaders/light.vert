#version 450
#extension GL_EXT_debug_printf : enable
#include "common.h"

layout (location = 0) out vec3 v3OutPosition;
layout (location = 1) out vec3 v3OutNormal;

void main() {
	DrawData dd = draws[pc.uidrawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform td = transforms[dd.uiTransformIndex];

	vec4 v4Position = td.m4Model * vec4(vert.v3Position, 1.0f);
	gl_Position = pc.m4ViewProjection * v4Position;
	v3OutPosition = v4Position.xyz;

	vec4 v4Normal = td.m4ModelInvTr * vec4(vert.v3Normal, 0.0f);
	v3OutNormal = v4Normal.xyz;
}