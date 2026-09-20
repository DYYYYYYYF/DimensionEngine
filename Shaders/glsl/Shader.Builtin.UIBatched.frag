#version 450

layout (location = 0) in vec4 vColor;
layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform LocalUniformObject{
	vec4 diffuse_color;
}ObjectUbo;

// Samplers
const int SAMP_DIFFUSE = 0;
layout (set = 1, binding = 1) uniform sampler2D Samplers[1];

layout (location = 1) in struct dto{
	vec2 tex_coord;
}in_dto;

void main(){
    // 合批路径：颜色直接来自顶点属性（UI 元素 per-vertex 颜色），
    // 乘以材质 diffuse_color（合批材质恒白，保持 API 语义一致）。
    // alpha 混合由管线 blend state 完成（scfg blend=1），不再做 0.35 硬边 discard。
    FragColor = texture(Samplers[SAMP_DIFFUSE], in_dto.tex_coord) * vColor * ObjectUbo.diffuse_color;
}
