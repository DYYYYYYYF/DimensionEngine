Texture2D text1 : register(t0);

struct PSInput
{
	[[vk::location(0)]] float3 inColor				: COLOR0;
	[[vk::location(1)]] float2 texCoord				: TEXCOORD0;
};

float4 main(PSInput pin) : SV_TARGET
{
	return float4(pin.inColor, 1.0f);
}

