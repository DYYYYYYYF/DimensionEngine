struct UBO
{
    float4x4 proj;
    float4x4 view;
    float4 ambient_color;
    float3 view_position;
    int mode;
    float global_time;
};

struct PushConstant
{
    float4x4 model;
};

struct VSInput
{
    [[vk::location(0)]] float3 vPosition : VECTOR;
    [[vk::location(1)]] float3 vNormal   : NORMAL0;
    [[vk::location(2)]] float2 vTexCoord : TEXCOORD0;
    [[vk::location(3)]] float4 vColor    : COLOR0;
    [[vk::location(4)]] float4 vTangent  : POSITION0;
};

struct VSOutput
{
    float4 outPosition      : SV_POSITION;
    int outMode             : INT0;
    float2 outTexcoord      : TEXCOORD0;
    float3 outNormal        : NORMAL0;
    float4 outAmbientColor  : COLOR0;
    float3 outViewPosition  : VECTOR0;
    float3 outFragPosition  : VECTOR1;
    float3 outVertPosition  : VECTOR2;
    float4 outColor         : COLOR1;
    float4 outTangent       : POSITION0;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

VSOutput main(VSInput input) 
{
    float4 Position = float4(input.vPosition, 1.0f);
    
	VSOutput output = (VSOutput)0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, Position)));
    output.outColor = input.vColor;
	output.outTexcoord = input.vTexCoord;
    output.outAmbientColor = ubo.ambient_color;
    output.outViewPosition = ubo.view_position;
    output.outFragPosition = mul(push_constants.model, float4(input.vPosition, 1.0f)).xyz;
    output.outNormal = normalize(mul(float3x3(push_constants.model), input.vNormal));
    output.outTangent = float4(normalize(mul(push_constants.model, input.vTangent)).xyz, input.vTangent.w);

    output.outMode = ubo.mode;
    output.outVertPosition = output.outPosition / 255.0f;

	return output;
}