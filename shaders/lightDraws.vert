#version 450
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out vec4 outVertColor;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	Transform transform = transforms[dd.transformIndex];

	outVertColor = lights[pc.lightId].diffuse;

	gl_Position = pc.perspectiveProj * transform.modelView * vec4(vert.position, 1.0f);

}