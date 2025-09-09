#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_GOOGLE_include_directive: require

#include "common.h"

layout (location = 0) out flat uint outDrawId;

void main() {
	DrawData dd = draws[pc.drawId];
	Vertex vert = vertices[gl_VertexIndex];
	TextureData textureData = textures[dd.textureIndex];

	vec3 vertPos = vert.position;

	// we choose to perform out lighting computations in camera-space.
	if (textureData.heightTexture > 0 && pc.enableHeightMapping > 0) {
		vertPos += vert.normal * (texture(texSampler[textureData.heightTexture], vert.UV).r * 0.13);
	}

	// we choose to perform out lighting computations in camera-space.
	gl_Position = lights[pc.lightId].viewProj * transforms[dd.transformIndex].model * vec4(vertPos, 1.0f);

}