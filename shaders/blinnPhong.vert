#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

layout(scalar, set = 0, binding = 0) readonly buffer Lights {
	Light lights[];
};

layout(scalar, set = 0, binding = 1) readonly buffer Materials {
	Material materials[];
};

// this effectively defines a type. a pointer to an array of vec3 positions. This is what we'll use to interpret the cube data provided using push constants and buffer device address
layout(scalar, buffer_reference) readonly buffer ptrBuffer {
	Vertex vertices[];
};

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat int outTexID;

layout(scalar, push_constant) uniform constants {
	ptrBuffer pVertexBuffer;
	mat4 perspectiveProj;
	mat4 modelView;
	mat4 mvInvTr;
	int texID;
} PushConstants;

void main() {
	ptrBuffer pVertBuffer = PushConstants.pVertexBuffer;
	outTexID = PushConstants.texID;
	outUV = pVertBuffer.vertices[gl_VertexIndex].UV;
	mat4 viewingTransform = PushConstants.perspectiveProj * PushConstants.modelView;
	
	Light light = lights[0];
	//vec3 lightViewPos = light.position

	// P is the vertex in eye-space, N is the normal in eye-space, L is the unit vector from the vertex to the light
	vec4 P = PushConstants.modelView * vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f);
	vec3 N = normalize((PushConstants.modelView * vec4(pVertBuffer.vertices[gl_VertexIndex].normal, 1.0f)).xyz);
	//vec3 L = ()

	gl_Position = viewingTransform * vec4(pVertBuffer.vertices[gl_VertexIndex].position, 1.0f);
	outVertColor = vec4(pVertBuffer.vertices[gl_VertexIndex].normal, 1.0f);
}