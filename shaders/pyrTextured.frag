#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 1) uniform sampler2D texSampler[];

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat int outTexID;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = inColor * texture(texSampler[outTexID], inUV);
}
