#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vPosition;
layout (location = 3) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor; 

    vec4 pointLightPos;
    vec4 lightSpecular;
} sceneData;

void main(){

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(sceneData.sunlightDirection.xyz);
    float strength = 1 - dot(normal, lightDir) * 2;
    vec3 color = vec3(strength, strength, strength);

    outFragColor = vec4(color, 1.0f);
}
