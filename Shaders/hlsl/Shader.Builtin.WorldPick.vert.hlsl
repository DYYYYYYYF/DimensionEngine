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
    [[vk::location(1)]] float3 vNormal   : NORMAL0;
    [[vk::location(2)]] float2 vTexcoord : TEXCOORD0;
    [[vk::location(3)]] float4 vColor    : COLOR;
    [[vk::location(4)]] float4 vTangent  : TANGENT;
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
    output.position = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 1.0f))));
    // 消费全部顶点属性（渲染无关，仅保证 SPIR-V 保留全部输入，消除校验层警告）
    output.debugAttribs = float4(input.vNormal, input.vTexcoord.x) + float4(input.vColor.xy, input.vTangent.xy);
    return output;
}