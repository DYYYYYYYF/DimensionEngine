struct UBO {
    column_major matrix proj;
    column_major matrix view;
};

struct PushConstant
{
    column_major matrix model;
};

struct VSInput
{
    [[vk::location(0)]] float3 vPosition : POSITION0;
    [[vk::location(1)]] float2 vTexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 outPosition : SV_POSITION;
    [[vk::location(0)]] float4 outColor : COLOR0;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
};

[[vk::binding(0, 0)]] UBO ubo;
[[vk::push_constant]] PushConstant push_constants;

VSOutput main(VSInput input) 
{
	VSOutput output = (VSOutput)0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 1.0f))));
    output.outColor = float4(1, 1, 1, 1);
	output.texCoord = input.vTexCoord;

	return output;
}