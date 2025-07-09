// GBuffer.vert.hlsl - G-Buffer顶点着色器

// 全局uniform对象 - 参考原始GLSL GlobalUniformObject
struct GlobalUniformObject
{
    float4x4 projection;
    float4x4 view;
    float4 ambient_color;
    float3 view_position;
    int mode;
    float global_time;
};

// 推送常量结构
struct PushConstants
{
    float4x4 model;
};

// 顶点着色器输入
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
    [[vk::location(0)]] int    out_mode        : INT0;
    [[vk::location(1)]] float2 vTexcoord       : TEXCOORD0;
    [[vk::location(2)]] float3 vNormal         : NORMAL0;
    [[vk::location(3)]] float3 vViewPosition   : VECTOR0;
    [[vk::location(4)]] float3 vWorldPosition  : VECTOR1;
    [[vk::location(5)]] float4 vColor          : COLOR0;
    [[vk::location(6)]] float4 vTangent        : TANGENT0;
    [[vk::location(7)]] float3 vBitangent      : VECTOR2;
};

// 资源绑定
[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformObject> GlobalUBO;
[[vk::push_constant]] ConstantBuffer<PushConstants> PushConstant;

// 顶点着色器主函数
VSOutput main(VSInput input)
{
    VSOutput output;
    
    // 计算世界空间位置
    float4 worldPosition = mul(PushConstant.model, float4(input.vPosition, 1.0f));
    output.vWorldPosition = worldPosition.xyz;
    
    // 传递纹理坐标和颜色
    output.vTexcoord = input.vTexcoord;
    output.vColor = input.vColor;
    
    // 变换法线到世界空间
    float3x3 normalMatrix = (float3x3)PushConstant.model;
    output.vNormal = normalize(mul(normalMatrix, input.vNormal));

    // 计算(副)切线
    output.vTangent = float4(normalize(mul(normalMatrix, input.vTangent.xyz)), input.vTangent.w);
    output.vBitangent = cross(output.vNormal, output.vTangent.xyz) * input.vTangent.w;
    
    // 输出模式
    output.out_mode = GlobalUBO.mode;

    // 摄像机位置
    output.vViewPosition = GlobalUBO.view_position;
    
    // 计算最终位置
    output.position = mul(GlobalUBO.projection, mul(GlobalUBO.view, worldPosition));
    
    return output;
}