struct VSInput {
	[[vk::location(0)]] float3 vPosition	: POSITION0;
	[[vk::location(1)]] float3 vNormal		: NORMAL0;
	[[vk::location(2)]] float3 vColor		: COLOR0;
	[[vk::location(3)]] float2 vTexCoord	: TEXCOORD0;
};

struct VSOutput {
	float4 outPosition								: SV_POSITION;
	[[vk::location(0)]] float3 outColor				: COLOR0;
	[[vk::location(1)]] float2 texCoord				: TEXCOORD0;
};

struct UBO {
	float4x4 view;
	float4x4 proj;
};

cbuffer ubo : register(b0, space0) { UBO ubo; }

VSOutput main(VSInput input) {
	VSOutput output = (VSOutput)0;
	output.outPosition = ubo.proj * ubo.view * float4(input.vPosition, 1.0f);
	output.outColor = input.vColor;
	output.texCoord = input.vTexCoord;

	return output;
}