#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(quads, equal_spacing, ccw) in;

#include "common.h"

layout(location = 0) in vec2 inTexCoords[];
layout(location = 0) out vec2 outTexCoords;

void main() {
	
	DrawData dd = draws[pc.drawId];
	Transform transform = transforms[dd.transformIndex];

	// ij where i refers to the column and j to the row
	vec3 p00 = gl_in[0].gl_Position.xyz;
	vec3 p10 = gl_in[1].gl_Position.xyz;
	vec3 p20 = gl_in[2].gl_Position.xyz;
	vec3 p30 = gl_in[3].gl_Position.xyz;
	vec3 p01 = gl_in[4].gl_Position.xyz;
	vec3 p11 = gl_in[5].gl_Position.xyz;
	vec3 p21 = gl_in[6].gl_Position.xyz;
	vec3 p31 = gl_in[7].gl_Position.xyz;
	vec3 p02 = gl_in[8].gl_Position.xyz;
	vec3 p12 = gl_in[9].gl_Position.xyz;
	vec3 p22 = gl_in[10].gl_Position.xyz;
	vec3 p32 = gl_in[11].gl_Position.xyz;
	vec3 p03 = gl_in[12].gl_Position.xyz;
	vec3 p13 = gl_in[13].gl_Position.xyz;
	vec3 p23 = gl_in[14].gl_Position.xyz;
	vec3 p33 = gl_in[15].gl_Position.xyz;

	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	// Cubic Bezier blend functions
	float bu0 = (1.0f-u)  * (1.0f-u)  * (1.0f-u);		//(1-u)^3
	float bu1 = 3.0f * u * (1.0f-u) * (1.0f-u);			//3u(1-u)^2  
	float bu2 = 3.0f * u * u * (1.0f-u);				//3u^2(1-u)
	float bu3 = u * u * u;								//u^3
	float bv0 = (1.0f-v)  * (1.0f-v)  * (1.0f-v);		//(1-v)^3
	float bv1 = 3.0f * v * (1.0f-v) * (1.0f-v);			//3v(1-v)^2  
	float bv2 = 3.0f * v * v * (1.0f-v);				//3v^2(1-v)
	float bv3 = v * v * v;								//v^3

	vec3 outputPosition =
		  bu0 * ( bv0*p00 + bv1*p01 + bv2*p02 + bv3*p03 )
		+ bu1 * ( bv0*p10 + bv1*p11 + bv2*p12 + bv3*p13 )
		+ bu2 * ( bv0*p20 + bv1*p21 + bv2*p22 + bv3*p23 )
		+ bu3 * ( bv0*p30 + bv1*p31 + bv2*p32 + bv3*p33 );

	gl_Position = pc.perspectiveProj * transform.modelView * vec4(outputPosition, 1.0f);
	//gl_Position = pc.perspectiveProj * transform.modelView * vec4(u, 0, v, 1.0f);
}