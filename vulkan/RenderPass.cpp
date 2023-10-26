#include "RenderPass.hpp"
#include "vulkan/vulkan_handles.hpp"

using namespace VkCore;

RenderPass::RenderPass(){
    _VkDevice = nullptr;
    _VkPhyDevice = nullptr;
}
RenderPass::~RenderPass(){}

vk::Format RenderPass::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const {
    CHECK(_VkPhyDevice);

    for(vk::Format format: candidates){
        vk::FormatProperties props;
        _VkPhyDevice.getFormatProperties(format, &props);
        if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features){
            return format;
        } else if(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features){
            return format;
        } else {
            throw std::runtime_error("Find Supported Format Failed...");
        }
    }
    vk::Format fail;
    return fail;
}

vk::Format RenderPass::FindDepthFormat() const {
    return FindSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32},vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::RenderPass RenderPass::CreateRenderPass(){
    CHECK(_VkDevice);
    CHECK(_VkPhyDevice);

    vk::RenderPassCreateInfo info;
    //颜色附件
    vk::AttachmentDescription attchmentDesc;
    attchmentDesc.setSamples(vk::SampleCountFlagBits::e1)
                 .setLoadOp(vk::AttachmentLoadOp::eClear)
                 .setStoreOp(vk::AttachmentStoreOp::eStore)
                 .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                 .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                 .setFormat(_SupportInfo.format.format)
                 .setInitialLayout(vk::ImageLayout::eUndefined)
                 .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
    info.setAttachments(attchmentDesc);
    
    vk::AttachmentReference colRefer;
    colRefer.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    colRefer.setAttachment(0);

       //深度附件
    // vk::AttachmentDescription depthAttchDesc;
    // depthAttchDesc.setFormat(FindDepthFormat())
    //               .setSamples(vk::SampleCountFlagBits::e1)
    //               .setLoadOp(vk::AttachmentLoadOp::eClear)
    //               .setStoreOp(vk::AttachmentStoreOp::eDontCare)
    //               .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
    //               .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
    //               .setInitialLayout(vk::ImageLayout::eUndefined)
    //               .setFinalLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal);
    // vk::AttachmentReference depthRef;
    // depthRef.setAttachment(1)
    //         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    //
    vk::SubpassDescription subpassDesc;
    subpassDesc.setColorAttachments(colRefer);
    // subpassDesc.setPDepthStencilAttachment(&depthRef);
    subpassDesc.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    // std::array<vk::AttachmentDescription, 2> attachments = {attchmentDesc, depthAttchDesc};
    std::array<vk::AttachmentDescription, 1> attachments = {attchmentDesc};
    info.setSubpassCount(1)
        .setSubpasses(subpassDesc)
        .setAttachmentCount(attachments.size())
        .setAttachments(attachments);
        
    _VkRenderPass = _VkDevice.createRenderPass(info);
    CHECK(_VkRenderPass);
    return _VkRenderPass;
}
