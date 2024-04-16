
struct UBO {
	row_major matrix view;
    row_major matrix proj;
    float3 viewPos;
};

struct PushConstant
{
    float4 data;
    row_major matrix model;
};

struct VSInput
{
    [[vk::location(0)]] float3 vPosition : POSITION0;
    [[vk::location(1)]] float3 vNormal : NORMAL0;
    [[vk::location(2)]] float3 vColor : COLOR0;
    [[vk::location(3)]] float2 vTexCoord : TEXCOORD0;

};

struct VSOutput
{
    float4 outPosition : SV_POSITION;
    [[vk::location(0)]] float3 outColor : COLOR0;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
};

[[vk::binding(0)]] UBO ubo : register(b0);
[[vk::push_constant]] PushConstant push_constants;

VSOutput main(VSInput input) 
{
	VSOutput output = (VSOutput)0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 1.0f))));
	output.outColor = input.vColor;
	output.texCoord = input.vTexCoord;

	return output;
}