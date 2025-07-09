#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexcoord;
layout (location = 3) in vec4 vColor;
layout (location = 4) in vec4 vTangent;

layout (set = 0, binding = 0, std140) uniform GlobalUniformObject{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;  
    vec3 view_position;  
    int mode;
    float global_time;  
}GlobalUBO;

layout (push_constant) uniform PushConstants{
    mat4 model;
}PushConstant;

layout (location = 0) out int out_mode;
layout (location = 1) out struct out_dto{
    vec2 vTexcoord;
    vec3 vNormal;
    vec3 vViewPosition;
    vec3 vWorldPosition;
    vec4 vColor;
    vec4 vTangent;
    vec3 vBitangent;
}OutDto;

void main(){
    // 计算世界空间位置
    vec4 worldPosition = PushConstant.model * vec4(vPosition, 1.0f);
    OutDto.vWorldPosition = worldPosition.xyz;
    
    // 传递纹理坐标和颜色
    OutDto.vTexcoord = vTexcoord;
    OutDto.vColor = vColor;
    
    // 变换法线到世界空间
    mat3 normalMatrix = mat3(PushConstant.model);
    OutDto.vNormal = normalize((PushConstant.model * vec4(vNormal, 1.0))).rgb;

    // 计算(副)切线
    OutDto.vTangent = vec4(normalize(normalMatrix * vTangent.xyz), vTangent.w);
    OutDto.vBitangent = cross(OutDto.vNormal, OutDto.vTangent.xyz) * vTangent.w;
    
    // 输出模式
    out_mode = GlobalUBO.mode;

    // 摄像机位置
    OutDto.vViewPosition = GlobalUBO.view_position;
    
    // 计算最终位置
    gl_Position = GlobalUBO.projection * GlobalUBO.view * worldPosition;
}