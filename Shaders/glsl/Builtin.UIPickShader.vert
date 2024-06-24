#version 450

layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec2 vTexcoord;

layout (set = 0, binding = 0) uniform GlobalUniformObject{
	mat4 projection;
	mat4 view;
}GlobalUBO;

layout (push_constant) uniform PushConstants{
	mat4 model;
}PushConstant;

void main(){
	gl_Position = GlobalUBO.projection * GlobalUBO.view * PushConstant.model * vec4(vPosition, 0.0f, 1.0f);
}
