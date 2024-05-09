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
    ivec2 u_resolution = textureSize(DiffuseSampler, 0);
    vec2 st = vec2(gl_FragCoord.xy / u_resolution) * 2.0f;
    st.x *= u_resolution.x / u_resolution.y;

    vec3 color = vec3(0.0f);

    st *= 20.0f;

    vec2 i_st = floor(st);
    vec2 f_st = fract(st);

    color += step(0.98f, f_st.x) + step(0.98f, f_st.y);

    FragColor = vec4(color,1.0);

   // FragColor = ObjectUbo.diffuse_color * texture(DiffuseSampler, in_dto.tex_coord);
}
