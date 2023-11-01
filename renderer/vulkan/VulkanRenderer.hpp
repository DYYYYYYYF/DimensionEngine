#pragma once

#ifdef DEBUG
#include <iostream>
#endif // DEBUG

#include <vector>
#include <vulkan/vulkan.hpp>
#include "../interface/IRendererImpl.hpp"
#include "../../application/Window.hpp"
#include "../../application/utils/EngineUtils.hpp"
#include "../../engine/EngineStructures.hpp"

#include "VkStructures.hpp"
#include "VkMesh.hpp"
/*
    Dont forget delete Init()
*/

using namespace engine;

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
        virtual void CreatePipeline(Material& mat) override;
        virtual void DrawPerFrame(RenderObject* first, int count) override;

        virtual void UpLoadMeshes(Mesh& mesh) override;

    public:
        vk::CommandBuffer AllocateCmdBuffer();
        void EndCmdBuffer(vk::CommandBuffer cmdBuf);

        // Buffer
        vk::Buffer CreateBuffer(uint64_t size, vk::BufferUsageFlags flag,
            vk::SharingMode mode = vk::SharingMode::eExclusive);
        vk::DeviceMemory AllocateMemory(MemRequiredInfo memInfo,
            vk::MemoryPropertyFlags flag = vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);

        // Image
        vk::Image CreateImage(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent);
        vk::ImageView CreateImageView(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect);

        // Uniform
        void UpdateViewMat(glm::mat4 view_matrix){
            _PushConstants.view = view_matrix;
        }

    protected:
        bool QueryQueueFamilyProp();
        bool InitQueue();
        void GetVkImages();
        void GetVkImageViews();
        void CreateDepthImage();
        void DrawObjects(vk::CommandBuffer cmd, RenderObject* first, int count);

        vk::ShaderModule CreateShaderModule(const char* shader_file);
        MemRequiredInfo QueryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag);
        MemRequiredInfo QueryImgReqInfo(vk::Image image, vk::MemoryPropertyFlags flag);

        void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
        void TransitionImageLayout(vk::Image image, vk::Format format,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

        bool IsContainStencilComponent(vk::Format format) {
            return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eX8D24UnormPack32;
        }

    private:
        void ResetProp();
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
        MeshPushConstants _PushConstants;

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

    };// class VulkanRenderer
}// namespace renderer
