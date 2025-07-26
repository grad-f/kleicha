#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable

struct Vertex {
	vec3 position;
	vec2 UV;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
};

struct DrawData {
		uint materialIndex;
		uint vertexOffset;
};

struct Transform {
		mat4 mv;
		mat4 mvInvTr;
};

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 mPos;
	vec3 mvPos;
};

layout(binding = 0, set = 0) readonly buffer Vertices {
	Vertex vertices[];
};

layout(binding = 1, set = 0) readonly buffer Draws {
	DrawData draws[];
};

// view * model
layout(binding = 0, set = 1) readonly buffer Transforms {
	Transform transforms[];
};

layout(binding = 1, set = 1) readonly buffer Materials {
	Material materials[];
};

layout(binding = 2, set = 1) readonly buffer Lights {
	Light lights[];
};

layout (location = 0) out vec4 outVertColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out flat uint outTexID;

layout(push_constant) uniform constants {
	mat4 perspectiveProj;
	uint drawId;
}pc;

void main() {
	DrawData dd = draws[pc.drawId];
	outTexID = dd.materialIndex;

	// we choose to perform out lighting computations in camera-space.
	vec4 P = transforms[pc.drawId].mv * vec4(vertices[gl_VertexIndex + dd.vertexOffset].position, 1.0f);
	vec3 N = normalize((transforms[pc.drawId].mvInvTr * vec4(vertices[gl_VertexIndex + dd.vertexOffset].normal, 1.0f)).xyz);
	vec3 L = normalize(lights[0].mvPos - P.xyz);

	// take the negative of the vertex pos vector and normalize to get direction vector from vertex to camera
	vec3 V = normalize(-P.xyz);
	// reflect direction vector from light to vertex and reflect about N
	vec3 R = reflect(-L, N);

	// define this here for now, we will store it in a global buffer in the future
	vec4 globalAmbient = vec4(0.7f, 0.7f, 0.7f, 1.0f);

	// object material determines how much of the ambient light is absorbed and reflected
	vec3 ambient = (globalAmbient * materials[pc.drawId].ambient + lights[0].ambient * materials[pc.drawId].ambient).xyz;

	// diffuse is similar to ambient but the angle between the normal and light direction vectors is also a factor and per color channel
	vec3 diffuse = lights[0].diffuse.xyz * materials[pc.drawId].diffuse.xyz * max(dot(N,L), 0.0f);
	
	vec3 specular = lights[0].specular.xyz * materials[pc.drawId].specular.xyz * pow(max(dot(R,V), 0.0f), materials[pc.drawId].shininess);



	//debugPrintfEXT("%f | %f | %f\n", lights[0].mvPos.x, lights[0].mvPos.y, lights[0].mvPos.z);

	gl_Position = pc.perspectiveProj * transforms[pc.drawId].mv * vec4(vertices[gl_VertexIndex + dd.vertexOffset].position, 1.0f);
	outUV = vertices[gl_VertexIndex + dd.vertexOffset].UV;
	outVertColor = vec4(ambient + diffuse + specular, 1.0f);
}