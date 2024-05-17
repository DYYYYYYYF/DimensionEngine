[[vk::binding(0, 1)]] Texture2D DiffuseTexture : register(t1);
[[vk::binding(1, 1)]] SamplerState DiffuseSampler : register(s1);

struct LocalUniformObject
{
    float4 DiffusrColor;
};

[[vk::binding(0)]] LocalUniformObject localuniform : register(b1);

struct PSInput
{
	[[vk::location(0)]] float4 inColor				: COLOR0;
	[[vk::location(1)]] float2 texCoord				: TEXCOORD0;
	
};

float4 main(PSInput pin) : SV_TARGET
{
    return DiffuseTexture.Sample(DiffuseSampler, pin.texCoord);
}

