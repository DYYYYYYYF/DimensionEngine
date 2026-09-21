struct VSSkyboxInput
{
    [[vk::location(0)]] float3 vPosition    : POSITION0;
    [[vk::location(1)]] float3 vNormal      : NORMAL0;
    [[vk::location(2)]] float2 vTexcoord    : TEXCOORD0;
    [[vk::location(3)]] float4 vColor       : COLOR;
    [[vk::location(4)]] float4 vTangent;
};

struct VSSkyboxOutput
{
    float4 outPosition : SV_Position;
    [[vk::location(0)]] float3 outTexcoord  : TEXCOORD0;
    // Consume all declared vertex attributes to match pipeline vertex input (validation OutputNotConsumed).
    [[vk::location(1)]] float4 debugAttribs : TEXCOORD1;
};

struct UBO
{
    float4x4 proj;
    float4x4 view;
};

[[vk::binding(0, 0)]] ConstantBuffer<UBO> Ubo;

VSSkyboxOutput main(VSSkyboxInput input)
{
    VSSkyboxOutput Output = (VSSkyboxOutput)0;
    Output.outTexcoord = input.vPosition;
    Output.outPosition = mul(Ubo.proj, mul(Ubo.view, float4(input.vPosition, 1.0f)));
    // Reference every declared attribute so the compiled SPIR-V keeps all vertex inputs.
    Output.debugAttribs = float4(input.vNormal, input.vTexcoord.x) + float4(input.vColor.xy, input.vTangent.xy);
    return Output;
}
