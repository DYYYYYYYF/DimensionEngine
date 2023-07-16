#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outTexCoord;
layout (location = 2) out vec3 baseLight;
layout (location = 3) out float lIntensity;
layout (location = 4) out vec3 lightPos;
layout (location = 5) out vec3 fragPos;
layout (location = 6) out vec3 normal;
layout (location = 7) out float specularIntensity;
layout (location = 8) out vec3 viewPos;

layout (binding = 0) uniform UniformBufferObj{
    mat4 model;         
    mat4 view;          
    mat4 projective;   
    vec3 baseLight;
    float intensity;
    vec3 lightPos;
    float specularIntensity;
    vec3 viewPos;
}ubo;

void main(){
    gl_Position = ubo.projective * ubo.view * ubo.model * vec4(inPos, 1);
    outColor = vec3(inColor);        //着色器顶点颜色
    baseLight = ubo.baseLight;
    lIntensity = ubo.intensity;
    
    lightPos = ubo.lightPos;
    fragPos = vec3(ubo.model * vec4(inPos, 1.0));;
    normal = mat3(transpose(inverse(ubo.model))) * (inPos + inNormal);

    specularIntensity = ubo.specularIntensity;
    viewPos = ubo.viewPos;
    outTexCoord = inTexCoord;   //纹理贴图
}
