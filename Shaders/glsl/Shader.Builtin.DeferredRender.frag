#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform local_uniform_obj{
	vec3 ID;
}objectUBO;

layout (location = 1) in struct dto{
	vec2 vTexcoord;
	vec3 vNormal;
	vec4 vAmbientColor;
	vec3 vViewPosition;
	vec3 vFragPosition;
	vec3 vVertPosition;
	vec4 vColor;
}in_dto;

// Samplers：diffuse, specular
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
const int SAMP_ROUGHNESS_METALLIC = 3;
layout (set = 1, binding = 1) uniform sampler2D Samplers[4];

void main(){
   vec3 RMA = texture(Samplers[SAMP_ROUGHNESS_METALLIC], in_dto.vTexcoord).rgb;

   FragColor = vec4(RMA, 1.0f);
}
