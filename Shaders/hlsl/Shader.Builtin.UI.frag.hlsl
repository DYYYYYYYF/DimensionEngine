struct LocalUniformObject
{
    float4 DiffusrColor;
};

struct PSInput
{
    [[vk::location(0)]] float4 inColor : COLOR0;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
};


[[vk::binding(0, 1)]] ConstantBuffer<LocalUniformObject> localuniform;
[[vk::binding(1, 1)]] Texture2D DiffuseTexture;
[[vk::binding(1, 1)]] SamplerState DiffuseSampler;

float4 main(PSInput pin) : SV_TARGET
{
    float4 FontColor = DiffuseTexture.Sample(DiffuseSampler, pin.texCoord);
    
    if (FontColor.a > 0.35f)
    {
        FontColor *= localuniform.DiffusrColor;
    }
    else
    {
        discard;
    }
    
    return FontColor;
}