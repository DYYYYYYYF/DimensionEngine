#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexcoord;
layout (location = 3) in vec4 vColor;
layout (location = 4) in vec4 vTangent;

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
	vec3 vVertPosition;
	vec4 vColor;
}OutDto;

void main(){
	OutDto.vTexcoord = vTexcoord;
	OutDto.vColor = vColor;
	OutDto.vAmbientColor = GlobalUBO.ambient_color;
	OutDto.vViewPosition = GlobalUBO.view_position;
	OutDto.vFragPosition = vec3(PushConstant.model * vec4(vPosition, 1.0f));
	OutDto.vNormal = normalize(mat3(PushConstant.model) * vNormal);
	gl_Position = GlobalUBO.projection * GlobalUBO.view * PushConstant.model * vec4(vPosition, 1.0f);
}
