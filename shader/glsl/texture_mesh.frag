#version 450

layout (location = 0) in vec3 inFragColor;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vPosition;
layout (location = 3) in vec2 texCoord;
layout (location = 4) in vec3 viewPos;

layout (set = 1, binding = 0) uniform sampler2D tex1;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 ambientDirection; //w for sun power

    vec4 pointLightPos;
    vec4 lightSpecular;
} sceneData;

void main(){

    float m_constant = float(1.0f);
    float m_linear =  float(0.09f);
    float m_quadratic = float(0.032f);

    vec3 normal = normalize(vNormal);
    vec3 vVeiwDir = normalize(viewPos - vPosition);
    vec3 lightDir = normalize(sceneData.pointLightPos.xyz - vPosition);
    vec3 vAmbientColor = sceneData.ambientColor.xyz;

    // Ambient
    vec3 ambient = sceneData.fogColor.xyz * sceneData.fogColor.w;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * vAmbientColor;

    // Specular
    vec3 reflectDir = reflect(-lightDir, vNormal);
    float spec = pow(max(dot(vVeiwDir, reflectDir), 0.0f), 32.0f);
    vec3 sepcular = spec * vAmbientColor * sceneData.lightSpecular.w;

    // Ligth attenuation (depend on distance)
    float distance = length(sceneData.pointLightPos.xyz - vPosition);    
    float attenuation = 1.0f / float((m_constant + m_linear * distance) + m_quadratic * (distance * distance));

    // Blend
    vec3 alpha = diffuse + ambient + sepcular;
    outFragColor = texture(tex1, texCoord) * vec4(alpha, 1.0f) *  attenuation;
}
