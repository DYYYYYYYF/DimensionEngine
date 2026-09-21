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
    float4 position : SV_POSITION;
    [[vk::location(1)]] float4 debugAttribs : TEXCOORD1;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.position = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 0.0f, 1.0f))));
    // 消费全部顶点属性（渲染无关，仅保证 SPIR-V 保留全部输入，消除校验层警告）
    output.debugAttribs = float4(input.vTexCoord, 0.0f, 1.0f);
    return output;
}