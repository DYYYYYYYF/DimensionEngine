#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexcoord;
layout (location = 3) in vec4 vColor;
layout (location = 4) in vec4 vTangent;

layout (set = 0, binding = 0) uniform GlobalUniformObject{
	mat4 projection;
	mat4 view;
}GlobalUBO;

layout (location = 0) out vec3 outTexcoord;

void main(){
	outTexcoord = vPosition;
	gl_Position = GlobalUBO.projection * GlobalUBO.view * vec4(vPosition, 1.0f);
}