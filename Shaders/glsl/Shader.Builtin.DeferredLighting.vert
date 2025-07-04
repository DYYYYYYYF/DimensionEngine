#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexcoord;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;
    vec3 view_position;
    int mode;
    float global_time;
}GlobalUBO;

layout (location = 0) out int out_mode;
layout (location = 1) out struct out_dto{
    vec2 vTexcoord;
}OutDto;

void main(){
    OutDto.vTexcoord = vTexcoord;
    out_mode = GlobalUBO.mode;
    
    // 直接输出NDC坐标，用于全屏四边形
    gl_Position = vec4(vPosition, 1.0);
}