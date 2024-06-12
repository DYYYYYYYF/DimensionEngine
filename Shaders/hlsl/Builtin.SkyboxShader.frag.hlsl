struct FSSkyboxInput
{
    [[vk::location(0)]] float3 vTexcoord;
};

// Samplers
const int SAMP_DIFFUSE = 0;
[[vk::binding(1, 0)]] TextureCube SkyboxTextures[];
[[vk::binding(1, 0)]] SamplerState SkyboxSamplers[];

float4 main(FSSkyboxInput input) : SV_TARGET
{
    return SkyboxTextures[SAMP_DIFFUSE].Sample(SkyboxSamplers[SAMP_DIFFUSE], input.vTexcoord);
}