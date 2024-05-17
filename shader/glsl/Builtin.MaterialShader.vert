#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexcoord;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
	mat4 projection;
	mat4 view;
	vec4 ambient_color;
	vec3 view_position;
}GlobalUBO;

layout (push_constant) uniform PushConstants{
	mat4 model;
}PushConstant;

layout (location = 1) out struct out_dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
}OutDto;

void main(){
	OutDto.vTexcoord = vec2(vTexcoord.x, 1.0 - vTexcoord.y);;
	OutDto.vNormal = mat3(PushConstant.model) * vNormal;
	OutDto.vAmbientColor = GlobalUBO.ambient_color;
	OutDto.vViewPosition = GlobalUBO.view_position;
	OutDto.vFragPosition = vec3(PushConstant.model * vec4(vPosition, 1.0f));

	gl_Position = GlobalUBO.projection * GlobalUBO.view * PushConstant.model * vec4(vPosition, 1.0f);
}
