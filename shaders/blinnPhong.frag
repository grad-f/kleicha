#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 4) uniform sampler2D texSampler[];

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat int inTexID;

layout (location = 0) out vec4 outColor;

void main() {
    //outColor = inColor;
    outColor = texture(texSampler[inTexID], inUV);
}
