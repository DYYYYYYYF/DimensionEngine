struct UBO
{
    column_major matrix proj;
    column_major matrix view;
};

struct PushConstant
{
    column_major matrix model;
};

struct VSInput
{
    [[vk::location(0)]] float2 vPosition : POSITION0;
    [[vk::location(1)]] float2 vTexCoord : TEXCOORD0;
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
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 0.0f, 1.0f))));

    return output;
}