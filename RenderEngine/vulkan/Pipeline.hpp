#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

namespace VkCore{
class PipelineBuilder{
public:
    std::vector<vk::PipelineShaderStageCreateInfo> _ShaderStages;
    vk::PipelineVertexInputStateCreateInfo _VertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo _InputAssembly;
    vk::Viewport _Viewport;
    vk::Rect2D _Scissor;
    vk::PipelineRasterizationStateCreateInfo _Rasterizer;
    vk::PipelineColorBlendAttachmentState _ColorBlendAttachment;
    vk::PipelineMultisampleStateCreateInfo _Mutisampling;
    vk::PipelineLayout _PipelineLayout;

    vk::Pipeline BuildPipeline(vk::Device device, vk::RenderPass renderpass);
}; // PipelineBuilder
} // VkCore
