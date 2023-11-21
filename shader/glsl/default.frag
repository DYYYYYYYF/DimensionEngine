#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor; 
} sceneData;

void main(){
     vec3 ambientColor = 0.5 * inFragColor + 0.5 * sceneData.ambientColor.xyz;
     outFragColor = vec4(ambientColor, 1.0f);
}
