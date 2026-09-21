// Shadow.vert.hlsl - Shadow深度通道顶点着色器

// 全局uniform对象
struct GlobalUniformObject
{
    float4x4 light_space_matrix;
};

// 推送常量结构
struct PushConstants
{
    float4x4 model;
};

// 顶点着色器输入（与引擎统一顶点格式一致，全属性消费避免校验层 OutputNotConsumed 警告）
struct VSInput
{
    [[vk::location(0)]] float3 vPosition  : POSITION;
    [[vk::location(1)]] float3 vNormal    : NORMAL;
    [[vk::location(2)]] float2 vTexcoord  : TEXCOORD0;
    [[vk::location(3)]] float4 vColor     : COLOR;
    [[vk::location(4)]] float4 vTangent   : TANGENT;
};

// 顶点着色器输出 - 对应片段着色器输入
struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(1)]] float4 debugAttribs : TEXCOORD1;
};

// 资源绑定
[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformObject> GlobalUBO;
[[vk::push_constant]] ConstantBuffer<PushConstants> PushConstant;

// 顶点着色器主函数
VSOutput main(VSInput input)
{
    VSOutput output;

    // 计算光源空间位置(Shadow深度写入)
    output.position = mul(GlobalUBO.light_space_matrix, mul(PushConstant.model, float4(input.vPosition, 1.0f)));

    // 消费全部顶点属性（渲染无关，仅保证 SPIR-V 保留全部输入，消除校验层警告）
    output.debugAttribs = float4(input.vNormal, input.vTexcoord.x) + float4(input.vColor.xy, input.vTangent.xy);

    return output;
}
