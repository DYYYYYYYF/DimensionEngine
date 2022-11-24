#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec4 FragColor;
layout (binding = 1) uniform sampler2D texSampler;
void main(){
    //Final Out
    FragColor = texture(texSampler,inTexCoord) * inColor;

    //TestDemo
    // FragColor =  vec4(lIntensity, 1.0) * inColor;
    // FragColor = texture(texSampler, inTexCoord) * vec4(specular, 1.0);
}