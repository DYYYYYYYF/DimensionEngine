struct LocalUniformObject
{
    float3 IDColor;
};

[[vk::binding(0, 1)]] LocalUniformObject localuniform;

float4 main() : SV_TARGET
{
    return float4(localuniform.IDColor, 1.0f);
}