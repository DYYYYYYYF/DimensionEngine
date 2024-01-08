#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <stb_image.h>
#include "VkTextrue.hpp"

using namespace renderer;

bool renderer::LoadImageFromFile(VulkanRenderer& renderer, const char* file, AllocatedImage& outImage){
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        FATAL("Failed to load texture file");
        return false;
    }

    vk::DeviceSize imageSize = (uint64_t)texWidth * texHeight * 4;
    vk::Buffer tempBuf = renderer.CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                                vk::SharingMode::eExclusive);
    ASSERT(tempBuf);
    MemRequiredInfo memInfo = renderer.QueryMemReqInfo(tempBuf, vk::MemoryPropertyFlagBits::eHostVisible|
                                vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::DeviceMemory tempMem = renderer.AllocateMemory(memInfo);
    ASSERT(tempMem);

    renderer._VkDevice.bindBufferMemory(tempBuf, tempMem, 0);
    void* data = renderer._VkDevice.mapMemory(tempMem, 0, imageSize);
    memcpy(data, pixels, imageSize);
    renderer._VkDevice.unmapMemory(tempMem);

    stbi_image_free(pixels);

    AllocatedImage newImage;
    vk::Extent3D imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    newImage.image = renderer.CreateImage(vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eTransferDst |
                                          vk::ImageUsageFlagBits::eSampled, imageExtent);
    ASSERT(newImage.image);

    memInfo = renderer.QueryImgReqInfo(newImage.image, vk::MemoryPropertyFlagBits::eDeviceLocal);
    newImage.memory = renderer.AllocateMemory(memInfo);
    ASSERT(newImage.memory);
    renderer._VkDevice.bindImageMemory(newImage.image, newImage.memory, 0);

    renderer.ImmediateSubmit([&](vk::CommandBuffer cmd){
        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setBaseArrayLayer(0)
            .setLevelCount(1)
            .setLayerCount(1);

        vk::ImageMemoryBarrier imageBarrier2Transfer;
        imageBarrier2Transfer.setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setImage(newImage.image)
            .setSubresourceRange(range)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setSrcAccessMask(vk::AccessFlagBits::eNone)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                            vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &imageBarrier2Transfer);

        vk::BufferImageCopy region;
        vk::ImageSubresourceLayers subSource;
        subSource.setBaseArrayLayer(0)
                 .setMipLevel(0)
                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                 .setLayerCount(1);
        region.setBufferImageHeight(0)
              .setBufferOffset(0)
              .setBufferRowLength(0)
              .setImageOffset({0,0,0})
              .setImageExtent(imageExtent)
              .setImageSubresource(subSource);

        cmd.copyBufferToImage(tempBuf, newImage.image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

        vk::ImageMemoryBarrier imageBarrier2Readable = imageBarrier2Transfer;
        imageBarrier2Readable.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                            vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &imageBarrier2Readable);
    });

    outImage = newImage;

    renderer._VkDevice.destroyBuffer(tempBuf);
    renderer._VkDevice.freeMemory(tempMem);

    INFO("Loaded %s", file);
    return true;
}
