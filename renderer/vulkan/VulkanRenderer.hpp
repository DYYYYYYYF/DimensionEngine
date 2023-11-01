#pragma once

#ifdef DEBUG
#include <iostream>
#endif // DEBUG

#include <vector>
#include <vulkan/vulkan.hpp>
#include "../interface/IRendererImpl.hpp"
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"

#include "VkStructures.hpp"
#include "VkMesh.hpp"
/*
    Dont forget delete Init()
*/

namespace renderer {

    class PipelineBuilder;

    class VulkanRenderer : public IRendererImpl {
    public:
        VulkanRenderer();
        virtual ~VulkanRenderer();
        virtual bool Init() override;
        virtual void Release() override;
        virtual void InitWindow() override;
        virtual void CreateInstance() override;
        virtual void PickupPhyDevice() override;
        virtual void CreateSurface() override;
        virtual void CreateDevice() override;
        virtual void CreateSwapchain() override;
        virtual void CreateRenderPass() override;
        virtual void CreateCmdPool() override;
        virtual void CreateFrameBuffers() override;
        virtual void InitSyncStructures() override;
        virtual void CreatePipeline() override;
        virtual void DrawPerFrame() override;

    public:
        vk::CommandBuffer AllocateCmdBuffer();
        void EndCmdBuffer(vk::CommandBuffer cmdBuf);
        vk::Buffer CreateBuffer(uint64_t size, vk::BufferUsageFlags flag,
            vk::SharingMode mode = vk::SharingMode::eExclusive);
        vk::DeviceMemory AllocateMemory(MemRequiredInfo memInfo,
            vk::MemoryPropertyFlags flag = vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

        // Image
        vk::Image CreateImage(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent);
        vk::ImageView CreateImageView(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect);

    protected:
        bool QueryQueueFamilyProp();
        bool InitQueue();
        void GetVkImages();
        void GetVkImageViews();
        void CreateDepthImage();

        vk::ShaderModule CreateShaderModule(const char* shader_file);
        MemRequiredInfo QueryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag);
        MemRequiredInfo QueryImgReqInfo(vk::Image image, vk::MemoryPropertyFlags flag);

        void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
        void UpLoadMeshes(Mesh& mesh);
        void TransitionImageLayout(vk::Image image, vk::Format format,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

        bool IsContainStencilComponent(vk::Format format) {
            return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eX8D24UnormPack32;
        }

    private:
        vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates,
            vk::ImageTiling tiling, vk::FormatFeatureFlags features);

        // Pipeline stages
        vk::PipelineShaderStageCreateInfo InitShaderStageCreateInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module);
        vk::PipelineVertexInputStateCreateInfo InitVertexInputStateCreateInfo();
        vk::PipelineInputAssemblyStateCreateInfo InitAssemblyStateCreateInfo(vk::PrimitiveTopology topology);
        vk::PipelineRasterizationStateCreateInfo InitRasterizationStateCreateInfo(vk::PolygonMode polygonMode);
        vk::PipelineMultisampleStateCreateInfo InitMultisampleStateCreateInfo();
        vk::PipelineColorBlendAttachmentState InitColorBlendAttachmentState();
        vk::PipelineLayoutCreateInfo InitPipelineLayoutCreateInfo();
        vk::PipelineDepthStencilStateCreateInfo InitDepthStencilStateCreateInfo();

    protected:
        QueueFamilyProperty _QueueFamilyProp;
        SwapchainSupport _SupportInfo;
        Queue _Queue;

        SDL_Window* _Window;
        vk::Instance _VkInstance;
        vk::PhysicalDevice _VkPhyDevice;
        vk::SurfaceKHR _SurfaceKHR;
        vk::Device _VkDevice;
        vk::SwapchainKHR _SwapchainKHR;

        std::vector<vk::Image> _Images;
        std::vector<vk::ImageView> _ImageViews;
        std::vector<vk::Framebuffer> _FrameBuffers;

        vk::RenderPass _VkRenderPass;
        vk::CommandPool _VkCmdPool;
        vk::CommandBuffer _VkCmdBuffer;
        vk::Semaphore _RenderSemaphore;
        vk::Semaphore _PresentSemaphore;
        vk::Fence _RenderFence;

        vk::PipelineLayout _PipelineLayout;
        vk::Pipeline _Pipeline;
        vk::ShaderModule _VertShader;
        vk::ShaderModule _FragShader;

        // Depth Image
        vk::ImageView _DepthImageView;
        AllocatedImage _DepthImage;

    public:
        void AddCount(){
            _FrameCount++;
        }

    private:
        void UploadTriangleMesh();
        Mesh _TriangleMesh;
        Mesh _MonkeyMesh;
        float _FrameCount = 1.0f;
    };// class VulkanRenderer
}// namespace renderer
