#version 450

// 根据scfg配置修改属性名称
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;
layout (location = 3) in vec4 in_color;
layout (location = 4) in vec4 in_tangent;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;  // 添加缺失的uniform
    vec3 view_position;  // 添加缺失的uniform
    int mode;
    float time;  // 改为time，匹配scfg配置
}GlobalUBO;

layout (push_constant) uniform PushConstants{
    mat4 model;
}PushConstant;

layout (location = 0) out int out_mode;
layout (location = 1) out struct out_dto{
    vec2 vTexcoord;
    vec3 vNormal;
    vec3 vWorldPosition;
    vec4 vColor;
    vec4 vTangent;
    vec3 vBitangent;
}OutDto;

void main(){
    // 计算世界空间位置
    vec4 worldPosition = PushConstant.model * vec4(in_position, 1.0f);
    OutDto.vWorldPosition = worldPosition.xyz;
    
    // 传递纹理坐标和颜色
    OutDto.vTexcoord = in_texcoord;
    OutDto.vColor = in_color;
    
    // 变换法线到世界空间
    mat3 normalMatrix = mat3(PushConstant.model);
    OutDto.vNormal = normalize(normalMatrix * in_normal);
    OutDto.vTangent = vec4(normalize(normalMatrix * in_tangent.xyz), in_tangent.w);
    
    // 计算副切线
    OutDto.vBitangent = cross(OutDto.vNormal, OutDto.vTangent.xyz) * in_tangent.w;
    
    // 输出模式
    out_mode = GlobalUBO.mode;
    
    // 计算最终位置
    gl_Position = GlobalUBO.projection * GlobalUBO.view * worldPosition;
}