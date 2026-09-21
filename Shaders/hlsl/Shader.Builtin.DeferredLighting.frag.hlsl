// DeferredLighting.frag.hlsl - 延迟光照片段着色器

// 全局uniform对象 - 参考原始GLSL GlobalUniformObject
struct GlobalUniformObject
{
    float4x4 light_space_matrix;
    // 以下光照参数全部由场景中的方向光 Actor 配置，经全局 UBO 上传
    float4 ambient_color;
    float4 light_direction;   // xyz = 光源指向场景的单位方向
    float4 light_color;
    float4 light_intensity;
    float global_time;
    float shadow_bias;
    float shadow_strength;    // 0 = 无阴影，1 = 全阴影

    // 点光源数组：数量与参数全部来自场景中的点光源 Actor（Editor/Config.json 的 PointLights 数组），
    // 逐帧上传；着色器内不存在任何硬编码光源。数组长度必须与引擎的 MAX_POINT_LIGHTS 一致（4）。
    uint point_light_count;              // 本帧有效点光源数量，0 表示场景中没有点光源
    float4 point_light_position[4];      // xyz = 世界空间位置
    float4 point_light_color[4];         // rgb = 颜色
    float4 point_light_attenuation[4];   // x = Constant, y = Linear, z = Quadratic, w = 强度
};

// 常量定义
static const int SAMP_ALBEDO = 0;
static const int SAMP_NORMAL = 1;
static const int SAMP_POSITION = 2;
static const int SAMP_SHADOW = 3;

// 实例uniform对象 - 参考原始GLSL InstanceUniformObject
struct InstanceUniformObject
{
    int debug_mode;
};

// 光源结构定义
struct DirectionalLight
{
    float3 direction;
    float4 color;
};

struct PointLight
{
    float3 position;
    float4 color;
    float fconstant;
    float linear_attenuation;
    float quadratic;
};

// 片段着色器输入 - 对应原始GLSL的in_dto结构
struct PSInput
{
    [[vk::location(1)]] float2 vTexcoord       : TEXCOORD1;
    [[vk::location(2)]] float4 ambient_color   : COLOR0;
    [[vk::location(3)]] float3 view_position   : VECTOR0;
};

// 像素着色器输出
struct PSOutput
{
    float4 FragColor : SV_TARGET0;
};

// 资源绑定
[[vk::binding(0, 0)]] ConstantBuffer<GlobalUniformObject> GlobalUBO;
[[vk::binding(0, 1)]] ConstantBuffer<InstanceUniformObject> InstanceUBO;
[[vk::binding(1, 1)]] Texture2D Samplers[4];
[[vk::binding(1, 1)]] SamplerState DefaultSampler[4];

// 方向光与点光源都不再使用硬编码常量：方向、颜色、强度、阴影偏置/强度来自 GlobalUBO 的方向光字段，
// 点光源来自 GlobalUBO 的 point_light_* 数组，数据均由场景中的光源 Actor 配置并逐帧上传。

// PBR常量
static const float PI = 3.14159265359;

// 阴影计算(3x3 PCF)
float CalculateShadow(float3 worldPosition, float3 worldNormal)
{
    // 法线偏移（normal-offset）：太阳贴近地平线时，光照方向与地面夹角很小，
    // 单个纹素覆盖的世界范围在光空间深度上的投影会被放大（斜度自阴影/痤疮），
    // 先把采样点沿法线推出一个世界尺度的小距离，再叠加斜率相关 bias。
    const float NORMAL_OFFSET_WORLD = 1.5f;
    float3 samplePosition = worldPosition + normalize(worldNormal) * NORMAL_OFFSET_WORLD;

    float4 lightSpacePos = mul(GlobalUBO.light_space_matrix, float4(samplePosition, 1.0));
    float3 proj = lightSpacePos.xyz / lightSpacePos.w;
    // light_space_matrix 由 zero_to_one=true 的正交矩阵生成：z 已落在 [0,1]（与阴影贴图里光栅化
    // 写入的深度同域），因此只有 xy 需要从 NDC[-1,1] 映射到贴图 uv[0,1]，z 必须原样保留。
    proj.xy = proj.xy * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 || proj.y < 0.0 || proj.y > 1.0) return 0.0;
    // bias 位于归一化光空间深度域 [0,1]，其世界尺度 = bias * (OrthoFar - OrthoNear)。
    // 当前 far-near = 800，故下限 0.0008 约合 0.64 世界单位，斜率项由 shadow_bias 提供。
    float bias = max(GlobalUBO.shadow_bias * (1.0 - dot(worldNormal, normalize(-GlobalUBO.light_direction.xyz))), 0.0008);
    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(2048.0, 2048.0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = Samplers[SAMP_SHADOW].Sample(DefaultSampler[SAMP_SHADOW], proj.xy + float2(x, y) * texelSize).r;
            shadow += (proj.z - bias > closest) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;                            // 3x3 PCF
}

// 方向光计算（只返回直射项：漫反射 + 高光；环境光由 main 统一累加，避免阴影项把环境光一起吃掉）
float4 CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 view_direction, float3 albedo, float metallic, float roughness)
{
    float fDiffuseFactor = max(dot(normal, -light.direction), 0.0f);
    
    float3 HalfDirection = normalize(-view_direction - light.direction);
    float SpecularFactor = pow(max(dot(HalfDirection, normal), 0.0f), lerp(128.0, 1.0, roughness));
    
    // 光源辐照度（颜色）必须进入漫反射项，否则太阳的暖色只会作用于高光
    float3 Radiance = light.color.rgb;
    float3 Diffuse = albedo * fDiffuseFactor * Radiance;
    float3 Specular = Radiance * SpecularFactor * (1.0 - metallic);
    
    // 强度来自光照 Actor 的 Intensity（方向光照 Actor 的 light_intensity.w == Intensity）
    return float4((Diffuse + Specular) * GlobalUBO.light_intensity.rgb, 1.0);
}

// 点光源计算（漫反射 + 高光 + 距离衰减；环境光同样由 main 统一累加）
float4 CalculatePointLight(PointLight light, float3 normal, float3 frag_position, float3 view_direction, float3 albedo, float metallic, float roughness)
{
    float3 LightDirection = normalize(light.position - frag_position);
    float Diff = max(dot(normal, LightDirection), 0.0f);
    
    float3 ReflectDirection = reflect(-LightDirection, normal);
    float Spec = pow(max(dot(view_direction, ReflectDirection), 0.0f), lerp(128.0, 1.0, roughness));
    
    // 计算衰减
    float Distance = length(light.position - frag_position);
    float Attenuation = 1.0f / (light.fconstant + light.linear_attenuation * Distance + light.quadratic * (Distance * Distance));
    
    // light.color.a = 该盏点光源 Actor 配置的 Intensity（RGB 为颜色）
    float3 Radiance = light.color.rgb;
    float Intensity = light.color.a;
    
    float3 Diffuse = albedo * Diff * Radiance * Attenuation;
    float3 Specular = Radiance * Spec * (1.0 - metallic) * Attenuation;
    
    // 每盏点光源使用各自的强度，不共享方向光的强度
    return float4((Diffuse + Specular) * Intensity, 1.0);
}

// 片段着色器主函数
PSOutput main(PSInput input)
{
    PSOutput output;
    
    // 从G-Buffer采样数据
    float4 albedoMetallic = Samplers[SAMP_ALBEDO].Sample(DefaultSampler[SAMP_ALBEDO], input.vTexcoord);
    float4 normalRoughness = Samplers[SAMP_NORMAL].Sample(DefaultSampler[SAMP_NORMAL], input.vTexcoord);
    float4 positionDepth = Samplers[SAMP_POSITION].Sample(DefaultSampler[SAMP_POSITION], input.vTexcoord);
    
    // 解码G-Buffer数据
    float3 albedo = albedoMetallic.rgb;
    float metallic = albedoMetallic.a;
    
    float3 worldNormal = normalize(normalRoughness.rgb * 2.0 - 1.0); // 解码法线
    float roughness = normalRoughness.a;
    
    float3 worldPosition = positionDepth.rgb;
    float depth = positionDepth.a;
    

    // 早期深度测试 - 如果深度为0说明没有几何体
    if (depth == 0.0)
    {
        discard;
    }
    
    // 根据模式渲染不同内容
    if (InstanceUBO.debug_mode == 1)
    {
        // 法线可视化
        output.FragColor = float4(worldNormal * 0.5 + 0.5, 1.0);
    }
    else if (InstanceUBO.debug_mode == 2)
    {
        // 材质属性可视化
        output.FragColor = float4(metallic, roughness, 0.0, 1.0);
    }
    else if (InstanceUBO.debug_mode == 3)
    {
        // 深度可视化
        output.FragColor = float4(depth, depth, depth, 1.0);
    }
    else if (InstanceUBO.debug_mode == 4)
    {
        // 反照率可视化
        output.FragColor = float4(albedo, 1.0);
    }
    else
    {
        // 标准光照计算：线性累加「环境光 + 方向光(含阴影) + 所有点光源」，最后整帧统一做 HDR 映射与 gamma 校正
        float3 viewDirection = normalize(input.view_position - worldPosition);

        // 方向光：参数来源于光照 Actor（GlobalUBO），不再使用硬编码常量
        DirectionalLight dir_light;
        dir_light.direction = normalize(GlobalUBO.light_direction.xyz);
        dir_light.color = GlobalUBO.light_color;

        float4 color = float4(0.0, 0.0, 0.0, 1.0);

        // 环境光（来自光照 Actor 配置的 AmbientColor）
        color.rgb += input.ambient_color.rgb * albedo;

        // 方向光阴影(仅作用于方向光直射分量)，强度由光照 Actor 配置的 shadow_strength 缩放
        float shadow = CalculateShadow(worldPosition, worldNormal) * clamp(GlobalUBO.shadow_strength, 0.0, 1.0);

        // 方向光（太阳）直射分量
        color += CalculateDirectionalLight(dir_light, worldNormal, viewDirection, albedo, metallic, roughness) * (1.0 - shadow);

        // 点光源：数量与参数全部来自场景中的点光源 Actor，逐盏累加（着色器内无任何硬编码光源）
        [loop] for (uint lightIndex = 0; lightIndex < GlobalUBO.point_light_count && lightIndex < 4; ++lightIndex) {
            PointLight pointLight;
            pointLight.position = GlobalUBO.point_light_position[lightIndex].xyz;
            // 强度随颜色一起传入（沿用 color.a 作为强度通道），与 LightActor 的 Intensity 对应
            pointLight.color = float4(GlobalUBO.point_light_color[lightIndex].rgb, GlobalUBO.point_light_attenuation[lightIndex].w);
            pointLight.fconstant = GlobalUBO.point_light_attenuation[lightIndex].x;
            pointLight.linear_attenuation = GlobalUBO.point_light_attenuation[lightIndex].y;
            pointLight.quadratic = GlobalUBO.point_light_attenuation[lightIndex].z;

            color += CalculatePointLight(pointLight, worldNormal, worldPosition, viewDirection, albedo, metallic, roughness);
        }

        // HDR 映射 + gamma 校正（整帧只做一次）
        float3 mapped = color.rgb / (color.rgb + 1.0);
        color.rgb = pow(mapped, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

        // 确保alpha为1
        output.FragColor = float4(color.rgb, 1.0);
    }
    
    return output;
}
