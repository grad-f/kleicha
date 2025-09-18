#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(quads, equal_spacing, ccw) in;

#include "common.h"

void main() {
	
	DrawData dd = draws[pc.drawId];
	Transform transform = transforms[dd.transformIndex];
	
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	gl_Position = pc.perspectiveProj * transform.modelView * vec4(u, 0, v, 1);
}