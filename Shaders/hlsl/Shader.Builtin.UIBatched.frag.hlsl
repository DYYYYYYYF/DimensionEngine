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
    float4 TextureColor = DiffuseTexture.Sample(DiffuseSampler, pin.texCoord);

    // 合批颜色路径：最终颜色 = 纹理采样 * 顶点色 * 材质色（材质色恒白）。
    // 不做 alpha test discard：合批 shader 走 pipeline blend=1 真混合，
    // 半透明元素（按钮 hover、渐变遮罩等）依赖 alpha 混合而非裁剪。
    float4 FinalColor = TextureColor * pin.inColor * localuniform.DiffusrColor;

    return FinalColor;
}
