struct UBO
{
    float4x4 proj;
    float4x4 view;
    float4 ambient_color;
    float3 view_position;
};

struct PushConstant
{
    float4x4 model;
};

struct VSInput
{
    [[vk::location(0)]] float3 vPosition : POSITION0;
    [[vk::location(1)]] float3 vNormal : NORMAL0;
    [[vk::location(2)]] float2 vTexCoord : TEXCOORD0;
    [[vk::location(3)]] float4 vColor : COLOR0;
    [[vk::location(4)]] float4 vTangent;
};

struct VSOutput
{
    float4 outPosition : SV_POSITION;
};

[[vk::binding(0, 0)]] UBO ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 1.0f))));
    return output;
}