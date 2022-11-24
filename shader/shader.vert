#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outTexCoord;


layout (binding = 0) uniform UniformBufferObj{
    mat4 model;         
    mat4 view;          
    mat4 projective;   
}ubo;

void main(){
    gl_Position = ubo.projective * ubo.view * ubo.model * vec4(inPos, 1);
    outColor = vec4(inColor, 1.0);        //着色器顶点颜色
    outTexCoord = inTexCoord;   //纹理贴图
}