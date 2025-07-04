#version 450

// 多个渲染目标输出
layout (location = 0) out vec4 out_albedo;     // RGB: 反照率, A: 金属度
layout (location = 1) out vec4 out_normal;     // RGB: 世界空间法线, A: 粗糙度
layout (location = 2) out vec4 out_position;   // RGB: 世界空间位置, A: 深度

layout (set = 1, binding = 0) uniform InstanceUniformObject{
    vec4 diffuse_color;
    float shininess;     // 添加scfg中定义的shininess
    float metallic;
    float roughness;
    float ambient_occlusion;
    float normal_intensity;
}InstanceUBO;

const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
const int SAMP_ROUGHNESS_METALLIC = 3;
layout (set = 1, binding = 1) uniform sampler2D Samplers[4];

layout (location = 0) flat in int in_mode;

layout (location = 1) in struct dto{
    vec2 vTexcoord;
    vec3 vNormal;
    vec3 vWorldPosition;
    vec4 vColor;
    vec4 vTangent;
    vec3 vBitangent;
}in_dto;

vec3 calculateWorldNormal();

void main(){
    // 采样基础纹理 - 使用新的纹理名称
    vec4 diffuseSample = texture(Samplers[SAMP_DIFFUSE], in_dto.vTexcoord);
    vec3 roughnessMetallicSample = texture(Samplers[SAMP_ROUGHNESS_METALLIC], in_dto.vTexcoord).rgb;
    
    // 计算反照率
    vec3 albedo = diffuseSample.rgb * InstanceUBO.diffuse_color.rgb * in_dto.vColor.rgb;
    
    // 从贴图或材质属性获取金属度和粗糙度
    float metallic = roughnessMetallicSample.b * InstanceUBO.metallic;  // 蓝色通道
    float roughness = roughnessMetallicSample.g * InstanceUBO.roughness; // 绿色通道
    
    // 计算世界空间法线
    vec3 worldNormal = calculateWorldNormal();
    
    // 根据模式输出不同内容（用于调试）
    if (in_mode == 1) {
        // 法线可视化模式
        out_albedo = vec4(worldNormal * 0.5 + 0.5, 1.0);
        out_normal = vec4(worldNormal * 0.5 + 0.5, roughness);
        out_position = vec4(in_dto.vWorldPosition, gl_FragCoord.z);
    }
    else if (in_mode == 2) {
        // 材质属性可视化模式
        out_albedo = vec4(metallic, roughness, InstanceUBO.ambient_occlusion, 1.0);
        out_normal = vec4(worldNormal * 0.5 + 0.5, roughness);
        out_position = vec4(in_dto.vWorldPosition, gl_FragCoord.z);
    }
    else if (in_mode == 3) {
        // 深度可视化模式
        float depth = gl_FragCoord.z;
        out_albedo = vec4(depth, depth, depth, 1.0);
        out_normal = vec4(worldNormal * 0.5 + 0.5, roughness);
        out_position = vec4(in_dto.vWorldPosition, depth);
    }
    else {
        // 标准G-Buffer输出模式
        out_albedo = vec4(albedo, metallic);
        out_normal = vec4(worldNormal * 0.5 + 0.5, roughness); // 法线编码到[0,1]范围
        out_position = vec4(in_dto.vWorldPosition, gl_FragCoord.z);
    }
}

vec3 calculateWorldNormal() {
    // 构建TBN矩阵
    vec3 Normal = normalize(in_dto.vNormal);
    vec3 Tangent = normalize(in_dto.vTangent.xyz);
    vec3 Bitangent = normalize(in_dto.vBitangent);
    
    // 确保切线垂直于法线
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    mat3 TBN = mat3(Tangent, Bitangent, Normal);
    
    // 从法线贴图采样并转换到切线空间 - 使用新的纹理名称
    vec3 localNormal = texture(Samplers[SAMP_NORMAL], in_dto.vTexcoord).rgb;
    localNormal = localNormal * 2.0 - 1.0; // [0,1] -> [-1,1]
    
    // 应用法线强度
    localNormal.xy *= InstanceUBO.normal_intensity;
    localNormal = normalize(localNormal);
    
    // 变换到世界空间
    vec3 worldNormal = normalize(TBN * localNormal);
    
    return worldNormal;
}