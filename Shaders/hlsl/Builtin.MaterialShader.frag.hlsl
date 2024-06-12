#include "Builtin.MaterialShader.structures"

[[vk::binding(1, 0)]] LocalUniformObject localuniform;
[[vk::binding(1, 1)]] Texture2D DiffuseTexture[];
[[vk::binding(1, 1)]] SamplerState DiffuseSampler[];

float4 main(PSInput pin) : SV_TARGET
{
    return DiffuseTexture[SAMP_DIFFUSE].Sample(DiffuseSampler[SAMP_DIFFUSE], pin.vTexcoord);
}

