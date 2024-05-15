#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexcoord;

layout (location = 0) out vec4 vColor;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
	mat4 projection;
	mat4 view;
	mat4 data1;
	mat4 data2;
}GlobalUBO;

layout (push_constant) uniform PushConstants{
	mat4 model;
}PushConstant;

layout (location = 1) out struct out_dto{
	vec2 tex_coord;
}OutDto;

void main(){
	OutDto.tex_coord = vTexcoord;
	gl_Position = GlobalUBO.projection * GlobalUBO.view * PushConstant.model * vec4(vPosition, 1.0f);
}
