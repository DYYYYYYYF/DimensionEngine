#version 450

layout (location = 0) in vec4 vColor;
layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform LocalUniformObject{
	vec4 diffuse_color;
}ObjectUbo;

// Samplers
layout (set = 1, binding = 1) uniform sampler2D DiffuseSampler;

layout (location = 1) in struct dto{
	vec2 tex_coord;
}in_dto;

float AlphaTest(float a, float b){
    if (a < b){
        return 1.0f;
    }

    return 0.0f;
}

void main(){
    

   FragColor = ObjectUbo.diffuse_color * texture(DiffuseSampler, in_dto.tex_coord);
}
