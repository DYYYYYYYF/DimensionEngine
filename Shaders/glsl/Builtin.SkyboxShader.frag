#version 450

layout (location = 0) in vec3 vTexcoord;
layout (location = 0) out vec4 FragColor;

// Samplers
const int SAMP_DIFFUSE = 0;
layout (set = 1, binding = 0) uniform samplerCube Samplers[1];

void main(){
	FragColor = texture(Samplers[SAMP_DIFFUSE], vTexcoord);
}