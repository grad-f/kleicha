#version 450
#extension GL_EXT_nonuniform_qualifier : require

#include "common.h"

layout(location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

void main() {
	
	DrawData dd = draws[pc.drawId];
	TextureData td = textures[dd.textureIndex];

	if(td.albedoTexture > 0)
		outColor = texture(texSampler[td.albedoTexture], inUV);
	else
		outColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}