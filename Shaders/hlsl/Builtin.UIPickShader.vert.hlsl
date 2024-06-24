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
};

[[vk::binding(0, 0)]] UBO ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

float4 main(VSInput input) : SV_POSITION
{
    return mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 0.0f, 1.0f))));
}