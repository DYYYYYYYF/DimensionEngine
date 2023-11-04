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

    constexpr unsigned int FRAME_OVERLAP = 2;

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
        virtual void UnloadMeshes(std::unordered_map<std::string, Mesh>& meshes) override{
            for (auto& mesh_map : meshes){
                Mesh& mesh = mesh_map.second; 
                _VkDevice.freeMemory(mesh.vertexBuffer.memory);
                _VkDevice.destroyBuffer(mesh.vertexBuffer.buffer);
        }
    }
        virtual void WaitIdel() override { _VkDevice.waitIdle(); }

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
        void UpdateViewMat(glm::mat4 view_matrix);

        void InitDescriptors();

    protected:
        bool QueryQueueFamilyProp();
        bool InitQueue();
        void GetVkImages();
        void GetVkImageViews();
        void CreateDepthImage();
        void DrawObjects(vk::CommandBuffer& cmd, RenderObject* first, int count);

        vk::ShaderModule CreateShaderModule(const char* shader_file);
        MemRequiredInfo QueryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag);
        MemRequiredInfo QueryImgReqInfo(vk::Image image, vk::MemoryPropertyFlags flag);

        void CopyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
        void TransitionImageLayout(vk::Image image, vk::Format format,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

        bool IsContainStencilComponent(vk::Format format) {
            return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eX8D24UnormPack32;
        }

        // Double Frame Buffer
        void AllocateFrameCmdBuffer();
        FrameData& GetCurrentFrame() {
            return _Frames[_FrameNumber % FRAME_OVERLAP];
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
        CamerData _Camera;

        GLFWwindow* _Window;
        vk::Instance _VkInstance;
        vk::PhysicalDevice _VkPhyDevice;
        vk::SurfaceKHR _SurfaceKHR;
        vk::Device _VkDevice;
        vk::SwapchainKHR _SwapchainKHR;

        std::vector<vk::Image> _Images;
        std::vector<vk::ImageView> _ImageViews;
        std::vector<vk::Framebuffer> _FrameBuffers;

        vk::RenderPass _VkRenderPass;
        vk::Pipeline _Pipeline;
        vk::PipelineLayout _PipelineLayout;

        // Depth Image
        vk::ImageView _DepthImageView;
        AllocatedImage _DepthImage;

        // Double frame buffer storage
        unsigned int _FrameNumber;
        FrameData _Frames[FRAME_OVERLAP];

        // Descriptor
        vk::DescriptorSetLayout _GlobalSetLayout;
        vk::DescriptorPool _DescriptorPool;

    };// class VulkanRenderer
}// namespace renderer
