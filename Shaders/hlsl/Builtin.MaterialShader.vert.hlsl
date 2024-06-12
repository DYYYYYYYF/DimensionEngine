#include "Builtin.MaterialShader.structures"

[[vk::binding(0, 0)]] UBO ubo;
[[vk::push_constant]] ConstantBuffer<PushConstant> push_constants;

VSOutput main(VSInput input) 
{
	VSOutput output = (VSOutput)0;
    output.outPosition = mul(ubo.proj, mul(ubo.view, mul(push_constants.model, float4(input.vPosition, 1.0f))));
    output.outColor = input.vColor;
	output.outTexcoord = input.vTexCoord;
    output.outAmbientColor = ubo.ambient_color;
    output.outViewPosition = ubo.view_position;
    output.outFragPosition = float3(mul(push_constants.model, float4(input.vPosition, 1.0f)));
    output.outNormal = normalize(mul(float3x3(push_constants.model), input.vNormal));
    output.outTangent = float4(normalize(mul(float3x3(push_constants.model), float3(input.vTangent))), input.vTangent.w);

	return output;
}