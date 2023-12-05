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

    float m_constant = float(1.0f);
    float m_linear =  float(0.09f);
    float m_quadratic = float(0.032f);

    // Ambient
    vec3 ambient = sceneData.ambientColor.xyz * sceneData.ambientColor.w;

    // Diffuse
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(sceneData.sunlightDirection.xyz);
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * sceneData.ambientColor.xyz;

    // Specular
    vec3 viewDir = normalize(sceneData.pointLightPos.xyz - vPosition);
    vec3 reflectDir = reflect(lightDir, vNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 16.0f);
    vec3 sepcular = spec * sceneData.ambientColor.xyz * sceneData.lightSpecular.xyz;

    // Ligth attenuation (depend on distance)
    float distance = length(sceneData.pointLightPos.xyz - vPosition);    
    float attenuation = 1.0f / float((m_constant + m_linear * distance) + m_quadratic * (distance * distance));

    // Blend
    vec3 alpha = diffuse + ambient + sepcular;
    vec3 ambientColor = alpha * inFragColor;

    outFragColor = vec4(ambientColor, 1.0f) * attenuation;
}
