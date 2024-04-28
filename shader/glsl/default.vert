#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 0) out vec4 vColor;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
	mat4 projection;
	mat4 view;
}GlobalUBO;

void main(){
	gl_Position = GlobalUBO.projection * GlobalUBO.view * vec4(vPosition, 1.0f);
}
