#version 450

// hard-code control points for now
const vec4 controlPoints[] = vec4[] (
	vec4(-1.0, 0.5, -1.0, 1.0),
	vec4(-0.5, 0.5, -1.0, 1.0),
	vec4( 0.5, 0.5, -1.0, 1.0),
	vec4( 1.0, 0.5, -1.0, 1.0),
	
	vec4(-1.0, 0.0, -0.5, 1.0),
	vec4(-0.5, 0.0, -0.5, 1.0),
	vec4( 0.5, 0.0, -0.5, 1.0),
	vec4( 1.0, 0.0, -0.5, 1.0),
	
	vec4(-1.0, 0.0,  0.5, 1.0),
	vec4(-0.5, 0.0,  0.5, 1.0),
	vec4( 0.5, 0.0,  0.5, 1.0),
	vec4( 1.0, 0.0,  0.5, 1.0),
	
	vec4(-1.0,-0.5,  1.0, 1.0),
	vec4(-0.5, 0.3,  1.0, 1.0),
	vec4( 0.5, 0.3,  1.0, 1.0),
	vec4( 1.0, 0.3,  1.0, 1.0)
);

layout(location = 0) out vec2 outTexCoord;

void main() {
	
	// map control points from [-1,1] to [0,1]
	vec4 controlPoint = controlPoints[gl_VertexIndex];
	outTexCoord = vec2((controlPoint.x + 1.0f) / 2.0f, (controlPoint.z + 1.0f) / 2.0f); 
	gl_Position = controlPoint;
}