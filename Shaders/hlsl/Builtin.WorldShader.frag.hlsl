#include "Builtin.WorldShader.structures"

[[vk::binding(1, 1)]] Texture2D Textures[];
[[vk::binding(1, 1)]] SamplerState Samplers[];

cbuffer Localuniform : register(b0, space1)
{
    LocalUniformObject Localuniform;
}

// Temp
static DirectionalLight dir_light =
{
    float3(0.0f, 0.57735f, 0.57735f),
	float4(0.8f, 0.8f, 0.8f, 1.0f)
};

static PointLight PointLight0 =
{
    float3(-5.5f, 0.0f, -5.5f),
	float4(0.0f, 1.0f, 0.0f, 1.0f),
	1.0f, // Constant
	0.35f, // Linear
	0.44f // Quadratic
};

static PointLight PointLight1 =
{
    float3(5.5f, 0.0f, -5.5f),
	float4(1.0f, 0.0f, 0.0f, 1.0f),
	1.0f, // Constant
	0.35f, // Linear
	0.44f // Quadratic
};
// Temp

float4 CalculateDirectionalLight(PSInput pin, DirectionalLight light, float3 normal, float3 view_direction);
float4 CalculatePointLight(PSInput pin, PointLight light, float3 normal, float3 frag_position, float3 view_direction);

float4 main(PSInput pin) : SV_TARGET
{
    float3 Normal = pin.vNormal;
    float3 Tangent = pin.vTangent.xyz;
    Tangent = (Tangent - dot(Tangent, Normal) * Normal);
    float3 Bitangent = cross(Normal, Tangent) * pin.vTangent.w;
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
	
	// Update the normal to use a sample from the normal map.
    float3 LocalNormal = 2.0f * Textures[SAMP_NORMAL].Sample(Samplers[SAMP_NORMAL], pin.vTexcoord).xyz - 1.0f;
    Normal = normalize(mul(LocalNormal, TBN));
	
    float3 vViewDirection = normalize(pin.vViewPosition - pin.vFragPosition);
    float4 FragColor = CalculateDirectionalLight(pin, dir_light, Normal, vViewDirection);
	
    FragColor += CalculatePointLight(pin, PointLight0, Normal, pin.vFragPosition, vViewDirection);
    FragColor += CalculatePointLight(pin, PointLight1, Normal, pin.vFragPosition, vViewDirection);
	
    return FragColor;
}

float4 CalculateDirectionalLight(PSInput pin, DirectionalLight light, float3 normal, float3 view_direction)
{
    float fDiffuseFactor = max(dot(normal, light.direction), 0.0f);
    
    float3 vHalfDirection = normalize(view_direction + light.direction);
    float fSpecularFactor = pow(max(dot(vHalfDirection, normal), 0.0f), Localuniform.Shiniess);
    
    float4 vDiffSamp = Textures[SAMP_DIFFUSE].Sample(Samplers[SAMP_DIFFUSE], pin.vTexcoord);
    float4 vAmbient = float4(pin.vAmbientColor.xyz * pin.vAmbientColor.w, 1.0f);
    float4 vDiffuse = float4(light.color.xyz * fDiffuseFactor, vDiffSamp.a);
    float4 vSpecular = float4(light.color.xyz * fSpecularFactor, vDiffSamp.a);
    
    vAmbient *= vDiffSamp;
    vDiffuse *= vDiffSamp;
    vSpecular *= float4(Textures[SAMP_SPECULAR].Sample(Samplers[SAMP_SPECULAR], pin.vTexcoord).xyz, vDiffuse.a);

    return (vAmbient + vDiffuse + vSpecular);
}

float4 CalculatePointLight(PSInput pin, PointLight light, float3 normal, float3 frag_position, float3 view_direction)
{
    float3 vLightDirection = normalize(light.position - frag_position);
    float fDiff = max(dot(normal, vLightDirection), 0.0f);
    
    float3 vHalfDirection = normalize(view_direction + vLightDirection);
    float fSpec = pow(max(dot(vHalfDirection, normal), 0.0f), Localuniform.Shiniess);
    
    // Calculate attenuation, or light falloff over distance.
    float fDistance = length(light.position - frag_position);
    float fAttenuation = 1.0f / (light.fconstant + light.linear_attenuation * fDistance + light.quadratic * fDistance * fDistance);
    
    float4 vAmbient = light.color * pin.vAmbientColor.w;
    float4 vDiffuse = light.color * fDiff;
    float4 vSpecular = light.color * fSpec;
    
    float4 vDiffSampler = Textures[SAMP_DIFFUSE].Sample(Samplers[SAMP_DIFFUSE], pin.vTexcoord);
    vDiffuse *= vDiffSampler;
    vAmbient *= vDiffSampler;
    vSpecular *= float4(Textures[SAMP_SPECULAR].Sample(Samplers[SAMP_SPECULAR], pin.vTexcoord).xyz, vDiffuse.a);
    
    // Fall off
    vDiffuse *= fAttenuation;
    vAmbient *= fAttenuation;
    vSpecular *= fAttenuation;
    
    return (vAmbient + vDiffuse + vSpecular);
}