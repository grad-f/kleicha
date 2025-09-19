#version 450

// specifies number of control points per patch and therefore the input and output arrays of the TCS and TES shaders
layout(vertices = 16) out;
/*

Tessellation Control Shader is invoked per control point. Each control point passes through the vertex shader and the Tessellation Control Shader.

The Tessellation Control Shader has access to all control points for a patch of control points processed by the vertex shader. In the current case, we only have 1 control point patch.

*/

layout(location = 0) in vec2 inUV[];
layout(location = 0) out vec2 outUV[];

void main() {	
	
	uint tessLevels = 32;

	if(gl_InvocationID == 0) {
		gl_TessLevelOuter[0] = tessLevels;
		gl_TessLevelOuter[1] = tessLevels;
		gl_TessLevelOuter[2] = tessLevels;
		gl_TessLevelOuter[3] = tessLevels;
		gl_TessLevelInner[0] = tessLevels;
		gl_TessLevelInner[1] = tessLevels;
	}

	outUV[gl_InvocationID] = inUV[gl_InvocationID];
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}