#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec2 texCoord;
layout (set = 1, binding = 0) uniform sampler2D tex1;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor; 
} sceneData;

void main(){
    vec2 u_resolution = vec2(600.f, 400.f);
    vec2 st = vec2(gl_FragCoord.xy / u_resolution);
    st.x *= u_resolution.x / u_resolution.y;

    vec3 color = vec3(.0);

    st *= 10.;

    vec2 i_st = floor(st);
    vec2 f_st = fract(st);

    color += step(.98,f_st.x) + step(.98,f_st.y);
    outFragColor = vec4(color * 0.5f, 1.0f);
}
