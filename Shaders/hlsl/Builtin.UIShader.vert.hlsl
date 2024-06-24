#include "Builtin.UIShader.structures"

[[vk::binding(0, 0)]] UBO ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 0.0f, 1.0f))));
    output.texCoord = input.vTexCoord;
    output.outColor = float4(1, 1, 1, 1);

    return output;
}