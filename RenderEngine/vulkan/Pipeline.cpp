#include "Pipeline.hpp"

using namespace VkCore;

vk::Pipeline PipelineBuilder::BuildPipeline(vk::Device device, vk::RenderPass renderpass){
    vk::PipelineViewportStateCreateInfo ViewportInfo;
    ViewportInfo.setViewportCount(1)
        .setPViewports(&_Viewport)
        .setScissorCount(1)
        .setPScissors(&_Scissor);

    vk::PipelineColorBlendStateCreateInfo ColorBlendInfo;
    ColorBlendInfo.setLogicOp(vk::LogicOp::eCopy)
        .setLogicOpEnable(VK_FALSE)
        .setAttachmentCount(1)
        .setAttachments(_ColorBlendAttachment);

    vk::GraphicsPipelineCreateInfo info;
    info.setStageCount(_ShaderStages.size())
        .setStages(_ShaderStages)
        .setPVertexInputState(&_VertexInputInfo)
        .setPInputAssemblyState(&_InputAssembly)
        .setPViewportState(&ViewportInfo)
        .setPRasterizationState(&_Rasterizer)
        .setPMultisampleState(&_Mutisampling)
        .setPColorBlendState(&ColorBlendInfo)
        .setLayout(_PipelineLayout)
        .setRenderPass(renderpass)
        .setSubpass(0)
        .setBasePipelineHandle(nullptr);

    vk::Pipeline newPipeline;
    auto res = device.createGraphicsPipelines(nullptr, 1, &info, nullptr, &newPipeline);
    if (res != vk::Result::eSuccess){
        return nullptr;
    }

    return newPipeline; 
}

