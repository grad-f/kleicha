#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out vec3 outSampleDir;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];

	outSampleDir = vec3(vert.position.x, -vert.position.y, vert.position.z);

	vec4 pos = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);
	gl_Position = vec4(pos.xy, 0.0f, pos.w);
}