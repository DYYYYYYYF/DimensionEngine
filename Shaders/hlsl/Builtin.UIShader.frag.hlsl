#include "Builtin.UIShader.structures"

[[vk::binding(0, 1)]] LocalUniformObject localuniform;
[[vk::binding(1, 1)]] Texture2D DiffuseTexture;
[[vk::binding(1, 1)]] SamplerState DiffuseSampler;

float4 main(PSInput pin) : SV_TARGET
{
    return DiffuseTexture.Sample(DiffuseSampler, pin.texCoord);
}