// GBuffer.frag.hlsl - G-Buffer片段着色器

// 常量定义
static const int SAMP_DIFFUSE = 0;
static const int SAMP_NORMAL = 1;
static const int SAMP_ROUGHNESS_METALLIC = 2;

// 局部uniform对象 - 参考原始GLSL InstanceUniformObject
struct LocalUniformObject
{
    float4 diffuse_color;
    float shininess;     
    float metallic;
    float roughness;
    float ambient_occlusion;
    float normal_intensity;
};

// 片段着色器输入 - 参考原始GLSL的in_dto结构
struct PSInput
{
    [[vk::location(0)]] int    in_mode         : INT0;
    [[vk::location(1)]] float2 vTexcoord       : TEXCOORD0;
    [[vk::location(2)]] float3 vNormal         : NORMAL0;
    [[vk::location(3)]] float3 vViewPosition   : VECTOR0;
    [[vk::location(4)]] float3 vWorldPosition  : VECTOR1;
    [[vk::location(5)]] float4 vColor          : COLOR0;
    [[vk::location(6)]] float4 vTangent        : TANGENT0;
    [[vk::location(7)]] float3 vBitangent      : VECTOR2;
};

// G-Buffer输出结构
struct GBufferOutput
{
    float4 out_albedo : SV_TARGET0;     // RGB: 反照率, A: 金属度
    float4 out_normal : SV_TARGET1;     // RGB: 世界空间法线, A: 粗糙度
    float4 out_position : SV_TARGET2;   // RGB: 世界空间位置, A: 深度
};

// 资源绑定
[[vk::binding(0, 1)]] ConstantBuffer<LocalUniformObject> InstanceUBO;
[[vk::binding(1, 1)]] Texture2D Samplers[];
[[vk::binding(1, 1)]] SamplerState DefaultSampler[];

// 计算世界空间法线的辅助函数
float3 calculateWorldNormal(PSInput input)
{
    // 构建TBN矩阵
    float3 Normal = normalize(input.vNormal);
    float3 Tangent = normalize(input.vTangent.xyz);
    float3 Bitangent = normalize(input.vBitangent);
    
    // 确保切线垂直于法线
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    
    // 从法线贴图采样并转换到切线空间
    float3 localNormal = Samplers[SAMP_NORMAL].Sample(DefaultSampler[SAMP_NORMAL], input.vTexcoord).rgb;
    localNormal = localNormal * 2.0 - 1.0; // [0,1] -> [-1,1]
    
    // 应用法线强度
    localNormal.xy *= InstanceUBO.normal_intensity;
    localNormal = normalize(localNormal);
    
    // 变换到世界空间
    float3 worldNormal = normalize(mul(TBN, localNormal));
    
    return worldNormal;
}

// 片段着色器主函数
GBufferOutput main(PSInput input)
{
    GBufferOutput output;
    
    // 采样基础纹理
    float4 diffuseSample = Samplers[SAMP_DIFFUSE].Sample(DefaultSampler[SAMP_DIFFUSE], input.vTexcoord);
    float3 roughnessMetallicSample = Samplers[SAMP_ROUGHNESS_METALLIC].Sample(DefaultSampler[SAMP_ROUGHNESS_METALLIC], input.vTexcoord).rgb;
    
    // 计算反照率
    float3 albedo = diffuseSample.rgb;
    
    // 从贴图或材质属性获取金属度、粗糙度和环境光遮蔽度
    float ao = roughnessMetallicSample.r;   
    float metallic = roughnessMetallicSample.g;  
    float roughness = roughnessMetallicSample.b; 

    // 计算世界空间法线
    float3 worldNormal = calculateWorldNormal(input);
    
    // 根据模式输出不同内容（用于调试）
    if (input.in_mode == 1)
    {
        // 法线可视化模式
        output.out_albedo = float4(worldNormal * 0.5 + 0.5, 1.0);
        output.out_normal = float4(worldNormal * 0.5 + 0.5, roughness);
        output.out_position = float4(input.vWorldPosition, 1.0); // 使用1.0作为有效深度
    }
    else if (input.in_mode == 2)
    {
        // 材质属性可视化模式
        output.out_albedo = float4(ao, roughness, metallic, 1.0);
        output.out_normal = float4(worldNormal * 0.5 + 0.5, roughness);
        output.out_position = float4(input.vWorldPosition, 1.0);
    }
    else if (input.in_mode == 3)
    {
        // 深度可视化模式
        float depth = input.vWorldPosition.z; // 使用世界空间Z坐标
        output.out_albedo = float4(depth, depth, depth, 1.0);
        output.out_normal = float4(worldNormal * 0.5 + 0.5, roughness);
        output.out_position = float4(input.vWorldPosition, depth);
    }
    else
    {
        // 标准G-Buffer输出模式
        output.out_albedo = float4(albedo, metallic);
        output.out_normal = float4(worldNormal * 0.5 + 0.5, roughness); // 法线编码到[0,1]范围
        output.out_position = float4(input.vWorldPosition, 1.0); // 使用1.0作为有效深度标记
    }
    
    return output;
}