#pragma once

#define _DEBUG_
#ifdef _DEBUG_
#include <iostream>
#endif // _DEBUG_

#include <vector>
#include <vulkan/vulkan.hpp>
#include <functional>
#include "PipelineBuilder.hpp"
#include "VkStructures.hpp"
#include "../interface/IRendererImpl.hpp"
#include "../../application/Window.hpp"
#include "../../application/EngineUtils.hpp"
#include "../../engine/EngineStructures.hpp"

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
        virtual void CreatePipeline(Material& mat, const char* vert_shader, const char* frag_shader, bool alpha) override;
        virtual void DrawPerFrame(RenderObject* first, int count, Particals* partical, int partical_count) override;

        virtual void UpLoadMeshes(Mesh& mesh) override;
    
        virtual void WaitIdel() override { _VkDevice.waitIdle(); }

    public:
        vk::CommandBuffer AllocateCmdBuffer();
        void EndCmdBuffer(vk::CommandBuffer cmdBuf);
        
        // Buffer
        vk::Buffer CreateBuffer(uint64_t size, vk::BufferUsageFlags flag,
            vk::SharingMode mode = vk::SharingMode::eExclusive);
        vk::DeviceMemory AllocateMemory(MemRequiredInfo memInfo);
      
        // Image
        vk::Image CreateImage(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent);
        vk::ImageView CreateImageView(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect);
        
        // Uniform
        void UpdatePushConstants(glm::mat4 view_matrix);
        void UpdateUniformBuffer();
        void UpdateDynamicBuffer();
        
        void InitDescriptors();
        void InitComputeDescriptors();
        void ImmediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function);
        MemRequiredInfo QueryMemReqInfo(vk::Buffer buf, vk::MemoryPropertyFlags flag);
        MemRequiredInfo QueryImgReqInfo(vk::Image image, vk::MemoryPropertyFlags flag);
        void BindTextureDescriptor(Material* mat, Texture* texture);
        void BindBufferDescriptor(Material* mat, Particals* partical);
        void CreateDrawLinePipeline(Material& mat, const char* vert_shader, const char* frag_shader);
        void CreateComputePipeline(Material& mat, const char* comp_shader);

        void UseTextureSet(bool val){_UseTextureSet = val;}

        void ReleaseBuffer(std::vector<Particals> particals){
            for (auto& partical : particals){
                _VkDevice.freeMemory(partical.writeStorageBuffer.memory);
                _VkDevice.destroyBuffer(partical.writeStorageBuffer.buffer);
                _VkDevice.freeMemory(partical.readStorageBuffer.memory);
                _VkDevice.destroyBuffer(partical.readStorageBuffer.buffer);
            }
         }

        void ReleaseMeshes(std::unordered_map<std::string, Mesh>& meshes) {
            for (auto& mesh_map : meshes) {
                Mesh& mesh = mesh_map.second;
                // Vertices
                _VkDevice.freeMemory(mesh.vertexBuffer.memory);
                _VkDevice.destroyBuffer(mesh.vertexBuffer.buffer);

                // Indices
                _VkDevice.freeMemory(mesh.indexBuffer.memory);
                _VkDevice.destroyBuffer(mesh.indexBuffer.buffer);
            }
        }

        void ReleaseMaterials(std::unordered_map<std::string, Material>& materials) {
            for (auto& material_map : materials) {
                Material& mat = material_map.second;
                _VkDevice.destroyPipeline(mat.pipeline);
                _VkDevice.destroyPipelineLayout(mat.pipelineLayout);
            }
        }

        void ReleaseTextures(std::unordered_map<std::string, Texture>& textures) {
            for (auto& texture : textures) {
                Texture& tex = texture.second;
                _VkDevice.destroyImage(tex.image.image);
                _VkDevice.freeMemory(tex.image.memory);
                _VkDevice.destroyImageView(tex.imageView);
            }
        }

        void MemoryMap(const AllocatedBuffer& src, void* dst, size_t offset, size_t length) {
            const void* data = _VkDevice.mapMemory(src.memory, offset, length);
            memcpy(dst, data, length);
            _VkDevice.unmapMemory(src.memory);
        }

    protected:
        bool QueryQueueFamilyProp();
        bool InitQueue();
        void GetVkImages();
        void GetVkImageViews();
        void CreateDepthImage();
        void DrawObjects(vk::CommandBuffer& cmd, RenderObject* first, int count);

        vk::ShaderModule CreateShaderModule(const char* shader_file);

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
        size_t PadUniformBuffeSize(size_t origin_size);
        vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates,
            vk::ImageTiling tiling, vk::FormatFeatureFlags features);

        void DrawGraphsicsPipeline(RenderObject* drawobjects, int count, int swapchain_index,
            vk::Semaphore& wait_semapore, vk::Semaphore& signal_semaphore);
        void DrawComputePipeline(Particals* particals, int partical_count);

        // Pipeline stages
        vk::PipelineShaderStageCreateInfo InitShaderStageCreateInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module);
        vk::PipelineVertexInputStateCreateInfo InitVertexInputStateCreateInfo();
        vk::PipelineInputAssemblyStateCreateInfo InitAssemblyStateCreateInfo(vk::PrimitiveTopology topology);
        vk::PipelineRasterizationStateCreateInfo InitRasterizationStateCreateInfo(vk::PolygonMode polygonMode);
        vk::PipelineMultisampleStateCreateInfo InitMultisampleStateCreateInfo();
        vk::PipelineColorBlendAttachmentState InitColorBlendAttachmentState(bool enable_Blend);
        vk::PipelineLayoutCreateInfo InitPipelineLayoutCreateInfo();
        vk::PipelineDepthStencilStateCreateInfo InitDepthStencilStateCreateInfo(bool enable_depth);

        vk::DescriptorSetLayoutBinding InitDescriptorSetLayoutBinding(vk::DescriptorType type, 
            vk::ShaderStageFlags stage, uint32_t binding);
        vk::WriteDescriptorSet InitWriteDescriptorBuffer(vk::DescriptorType type, vk::DescriptorSet dstSet, 
            vk::DescriptorBufferInfo* bufferInfo, uint32_t binding);
        vk::WriteDescriptorSet InitWriteDescriptorImage(vk::DescriptorType type, vk::DescriptorSet dstSet,
            vk::DescriptorImageInfo* imageInfo, uint32_t binding);
        vk::SamplerCreateInfo InitSamplerCreateInfo(vk::Filter filter, vk::SamplerAddressMode samplerAddressMode);
   

    public:
        vk::Device _VkDevice;

    protected:
        QueueFamilyProperty _QueueFamilyProp;
        SwapchainSupport _SupportInfo;
        Queue _Queue;
        MeshPushConstants _PushConstants;
        CamerData _Camera;
        UploadContext _UploadContext;

        // Scene dynamic buffer
        SceneData _SceneData;
        AllocatedBuffer _SceneParameterBuffer;

        GLFWwindow* _Window;
        vk::Instance _VkInstance;
        vk::PhysicalDevice _VkPhyDevice;
        vk::SurfaceKHR _SurfaceKHR;
        vk::SwapchainKHR _SwapchainKHR;

        std::vector<vk::Image> _Images;
        std::vector<vk::ImageView> _ImageViews;
        std::vector<vk::Framebuffer> _FrameBuffers;

        vk::RenderPass _VkRenderPass;
        vk::Pipeline _Pipeline;
        vk::Pipeline _DrawLinePipeline;
        vk::PipelineLayout _PipelineLayout;
        
        vk::Pipeline _ComputePipeline;
        vk::PipelineLayout _ComputePipelineLayout;

        // Depth Image
        vk::ImageView _DepthImageView;
        AllocatedImage _DepthImage;

        // Double frame buffer storage
        unsigned int _FrameNumber;
        FrameData _Frames[FRAME_OVERLAP];

        // Descriptor
        vk::DescriptorSetLayout _GlobalSetLayout;
        vk::DescriptorPool _DescriptorPool;
        vk::DescriptorSetLayout _TextureSetLayout;
        vk::DescriptorSet _TextureSet;

        vk::DescriptorSetLayout _ComputeSetLayout;
        vk::DescriptorSet _ComputeSet;
        
        // Texture
        vk::Sampler _TextureSampler;

    private:
        bool _UseTextureSet;

    };// class VulkanRenderer
}// namespace renderer
