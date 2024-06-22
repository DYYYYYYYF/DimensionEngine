#version 450

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform local_uniform_obj{
	vec3 ID;
}objectUBO;

void main(){
   FragColor = vec4(objectUBO.ID, 1.0f);
}
