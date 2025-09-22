#version 450
#include "common.h"

layout (location = 0) in vec3 v3InColor;

layout (location = 0) out vec3 v3OutColor;

void main() {
	v3OutColor = v3InColor;
}