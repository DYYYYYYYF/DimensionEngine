#include "VulkanShader.hpp"
#include "Systems/ResourceSystem.h"
#include "Core/EngineLogger.hpp"
#include "Platform/File.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "VulkanBackend.hpp"
#include "Core/Utils.hpp"

bool VulkanShader::Initialize() {
	if (Renderer == nullptr) {
		GLOG(Log::eError, "VulkanShader::Initialize Falied. Renderer ptr is nullptr please offer a valued ptr in construction.");
		return false;
	}

	VulkanBackend* vkRenderer = static_cast<VulkanBackend*>(Renderer->GetRenderBackend());
	VulkanContext& Context = vkRenderer->Context;
	vk::Device LogicalDevice = vkRenderer->Context.Device.GetLogicalDevice();
	vk::AllocationCallbacks* VkAllocator = vkRenderer->Context.Allocator;
	ASSERT(VkAllocator);

	// Static lookup table for our types->vulkan once.
	static vk::Format* Types = nullptr;
	static vk::Format t[11];
	if (!Types) {
		t[ShaderAttributeType::eShader_Attribute_Type_Float] = vk::Format::eR32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_2] = vk::Format::eR32G32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_3] = vk::Format::eR32G32B32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Float_4] = vk::Format::eR32G32B32A32Sfloat;
		t[ShaderAttributeType::eShader_Attribute_Type_Int8] = vk::Format::eR8Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt8] = vk::Format::eR8Uint;
		t[ShaderAttributeType::eShader_Attribute_Type_Int16] = vk::Format::eR16Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt16] = vk::Format::eR16Uint;
		t[ShaderAttributeType::eShader_Attribute_Type_Int32] = vk::Format::eR32Sint;
		t[ShaderAttributeType::eShader_Attribute_Type_UInt32] = vk::Format::eR32Uint;
		Types = t;
	}

	// Process attributes.
	uint32_t AttributeCount = (uint32_t)Attributes.size();
	uint32_t Offset = 0;
	for (uint32_t i = 0; i < AttributeCount; ++i) {
		// Setup the new attribute.
		vk::VertexInputAttributeDescription Attribute;
		Attribute.setLocation(i)
			.setBinding(0)
			.setOffset(Offset)
			.setFormat(Types[Attributes[i].type]);

		// Push into the config's attribute collection and add to the stride.
		Config.attributes[i] = Attribute;
		Offset += Attributes[i].size;
	}

	// Descriptor pool.
	if (Status == ShaderStatus::eShader_State_Uninitialized) {
		vk::DescriptorPoolCreateInfo PoolInfo;
		PoolInfo.setPoolSizeCount(2)
			.setPPoolSizes(Config.pool_sizes)
			.setMaxSets(Config.max_descriptor_set_count)
			.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

		// Create descriptor pool.
		DescriptorPool = LogicalDevice.createDescriptorPool(PoolInfo, VkAllocator);
		ASSERT(DescriptorPool);
	}

	if (!CompileShaderFile()) {
		GLOG(Log::eError, "VulkanShader::Reload: compile shader failed for shader '%s'.", Name.c_str());
		return false;
	}

	if (!CreateModule()) {
		GLOG(Log::eError, "VulkanShader::Initialize: create shader module failed for shader '%s'.", Name.c_str());
		return false;
	}

	if (!CreatePipeline()) {
		GLOG(Log::eError, "VulkanShader::Initialize: create pipeline failed for shader '%s'.", Name.c_str());
		return false;
	}

	return true;
}

bool VulkanShader::Reload() {
	if (Renderer == nullptr) {
		GLOG(Log::eError, "VulkanShader::Initialize Falied. Renderer ptr is nullptr please offer a valued ptr in construction.");
		return false;
	}

	VulkanBackend* vkRenderer = static_cast<VulkanBackend*>(Renderer->GetRenderBackend());
	VulkanContext& Context = vkRenderer->Context;
	vk::Device LogicalDevice = vkRenderer->Context.Device.GetLogicalDevice();
	vk::AllocationCallbacks* VkAllocator = vkRenderer->Context.Allocator;

	// Wait for frame. Can not reload when the shader in used.
	LogicalDevice.waitIdle();

	// Grab the UBO alignment requirement from the device.
	RequiredUboAlignment = Context.Device.GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	// Make sure the UBO is aligned according to device requirements.
	GlobalUboStride = PaddingAligned(GlobalUboSize, RequiredUboAlignment);
	UboStride = PaddingAligned(UboSize, RequiredUboAlignment);

	// Uniform buffer.
	vk::MemoryPropertyFlags DeviceLocalBits = Context.Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags();
	// TODO: max count should be configurable, or perhaps long term support of buffer resizing.
	size_t TotalBufferSize = GlobalUboStride + (UboStride * VULKAN_MAX_MATERIAL_COUNT);

	// Uniform buffer.
	Renderer->UnmapMemory(&UniformBuffer, 0, TotalBufferSize);
	MappedUniformBufferBlock = nullptr;
	Renderer->DestroyRenderbuffer(&UniformBuffer);

	// Pipeline
	Pipeline.Destroy(&vkRenderer->Context);

	// Shader modules.
	for (uint32_t i = 0; i < Config.stage_count; ++i) {
		LogicalDevice.destroyShaderModule(Stages[i].shader_module, VkAllocator);
	}

	if (!CompileShaderFile()) {
		GLOG(Log::eError, "VulkanShader::Reload: compile shader failed for shader '%s'.", Name.c_str());
		return false;
	}

	if (!CreateModule()) {
		GLOG(Log::eError, "VulkanShader::Reload: create shader module failed for shader '%s'.", Name.c_str());
		return false;
	}

	if (!CreatePipeline()) {
		GLOG(Log::eError, "VulkanShader::Reload: create pipeline failed for shader '%s'.", Name.c_str());
		return false;
	}

	return true;
}

void VulkanShader::Destroy(){
	VulkanBackend* vkRenderer = static_cast<VulkanBackend*>(Renderer->GetRenderBackend());
	VulkanContext& Context = vkRenderer->Context;

	vk::Device LogicalDevice = Context.Device.GetLogicalDevice();
	vk::AllocationCallbacks* VkAllocator = Context.Allocator;

	LogicalDevice.waitIdle();

	// Descriptor set layouts.
	for (uint32_t i = 0; i < Config.descriptor_set_count; ++i) {
		if (DescriptorSetLayouts[i]) {
			LogicalDevice.destroyDescriptorSetLayout(DescriptorSetLayouts[i], VkAllocator);
			DescriptorSetLayouts[i] = nullptr;
		}
	}

	// Descriptor pool
	if (DescriptorPool) {
		LogicalDevice.destroyDescriptorPool(DescriptorPool, VkAllocator);
		DescriptorPool = nullptr;
	}

	// Uniform buffer.
	Renderer->UnmapMemory(&UniformBuffer, 0, VK_WHOLE_SIZE);
	MappedUniformBufferBlock = nullptr;
	Renderer->DestroyRenderbuffer(&UniformBuffer);

	// Pipeline
	Pipeline.Destroy(&Context);

	// Shader modules.
	for (uint32_t i = 0; i < Config.stage_count; ++i) {
		LogicalDevice.destroyShaderModule(Stages[i].shader_module, VkAllocator);
	}

	// Destroy the configuration.
	Memory::Zero(&Config, sizeof(VulkanShaderConfig));

	// Free hash mem.
	HashMap.clear();

	// Reset status.
	Status = ShaderStatus::eShader_State_Not_Created;

	uint32_t SamplerCount = (uint32_t)GlobalTextureMaps.size();
	for (uint32_t i = 0; i < SamplerCount; ++i) {
		GlobalTextureMaps[i] = nullptr;
	}
	GlobalTextureMaps.clear();
	std::vector<TextureMap*>().swap(GlobalTextureMaps);
}

bool VulkanShader::CreateModule() {
	if (Renderer == nullptr) return false;
	Memory::Zero(Stages, sizeof(VulkanShaderStage) * VULKAN_SHADER_MAX_STAGES);

	VulkanBackend* vkRenderer = static_cast<VulkanBackend*>(Renderer->GetRenderBackend());
	for (uint32_t i = 0; i < Config.stage_count; ++i) {
		VulkanShaderConfig vkShaderConfig = Config;
		VulkanShaderStageConfig vkShaderStageConfig = Config.stages[i];
		VulkanShaderStage* vkShaderStage = &Stages[i];

		Resource BinaryResource;
		if (!ResourceSystem::Load(vkShaderStageConfig.filename, ResourceType::eResource_type_Binary, nullptr, &BinaryResource)) {
			GLOG(Log::eError, "Unable to create %s shader module for '%s'. Shader will be destroyed.", vkShaderStageConfig.filename, Name.c_str());
			return false;
		}

		Memory::Zero(&vkShaderStage->create_info, sizeof(vk::ShaderModuleCreateInfo));
		vkShaderStage->create_info.sType = vk::StructureType::eShaderModuleCreateInfo;
		// Use the resource's size and data directly.
		vkShaderStage->create_info.codeSize = BinaryResource.DataSize;
		vkShaderStage->create_info.pCode = (uint32_t*)BinaryResource.Data;

		vkShaderStage->shader_module = vkRenderer->Context.Device.GetLogicalDevice().createShaderModule(vkShaderStage->create_info, vkRenderer->Context.Allocator);
		ASSERT(vkShaderStage->shader_module);

		// Release the resource.
		ResourceSystem::Unload(&BinaryResource);

		// Shader stage info.
		Memory::Zero(&vkShaderStage->shader_stage_create_info, sizeof(vk::ShaderModuleCreateInfo));
		vkShaderStage->shader_stage_create_info.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		vkShaderStage->shader_stage_create_info.stage = vkShaderStageConfig.stage;
		vkShaderStage->shader_stage_create_info.module = vkShaderStage->shader_module;
		vkShaderStage->shader_stage_create_info.pName = "main";

		// Shader stage info.
		Memory::Zero(&vkShaderStage->shader_stage_create_info, sizeof(vk::ShaderModuleCreateInfo));
		vkShaderStage->shader_stage_create_info.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
		vkShaderStage->shader_stage_create_info.stage = vkShaderStageConfig.stage;
		vkShaderStage->shader_stage_create_info.module = vkShaderStage->shader_module;
		vkShaderStage->shader_stage_create_info.pName = "main";
	}

	return true;
}

bool VulkanShader::CompileShaderFile(bool writeToDisk/* = true*/){
	for (uint32_t i = 0; i < Config.stage_count; ++i) {
		VulkanShaderConfig vkShaderConfig = Config;
		VulkanShaderStageConfig vkShaderStageConfig = Config.stages[i];
		VulkanShaderStage* vkShaderStage = &Stages[i];

		// Read the resource.
		Resource BinaryResource;
		std::string ShaderFile = ResourceSystem::GetRootPath() + std::string("/")
			+ std::string(vkShaderConfig.stages[i].filename);

		File SPVFile(ShaderFile);
		if (!SPVFile.IsExist() || Status == ShaderStatus::eShader_State_Reloading){
			ShaderStage ShadercStage;
			switch (vkShaderStageConfig.stage)
			{
			case vk::ShaderStageFlagBits::eVertex:
				ShadercStage = ShaderStage::eShader_Stage_Vertex;
				break;
			case vk::ShaderStageFlagBits::eGeometry:
				ShadercStage = ShaderStage::eShader_Stage_Geometry;
				break;
			case vk::ShaderStageFlagBits::eFragment:
				ShadercStage = ShaderStage::eShader_Stage_Fragment;
				break;
			default:
				GLOG(Log::eError, "Unknown shader stage flag.");
				return false;
			}

			CompileShaderToSPV(vkShaderStageConfig.filename, ShadercStage);
		}
	}

	return true;
}

bool VulkanShader::CreatePipeline() {
	VulkanBackend* vkRenderer = static_cast<VulkanBackend*>(Renderer->GetRenderBackend());
	VulkanContext& Context = vkRenderer->Context;
	vk::Device LogicalDevice = vkRenderer->Context.Device.GetLogicalDevice();
	vk::AllocationCallbacks* VkAllocator = vkRenderer->Context.Allocator;
	ASSERT(VkAllocator);

	// Create descriptor set layouts.
	for (uint32_t i = 0; i < Config.descriptor_set_count; ++i) {
		if (!DescriptorSetLayouts[i]) {
			vk::DescriptorSetLayoutCreateInfo LayoutInfo;
			LayoutInfo.setBindingCount(Config.descriptor_sets[i].binding_count)
				.setPBindings(Config.descriptor_sets[i].bindings);
			DescriptorSetLayouts[i] = LogicalDevice.createDescriptorSetLayout(LayoutInfo, VkAllocator);
		}
		ASSERT(DescriptorSetLayouts[i]);
	}

	// TODO: This feels wrong to have these here, at least in this fashion. Should probably
	// Be configured to pull from someplace instead.
	// Viewport.
	vk::Viewport Viewport;
	Viewport.setX(0.0f)
		.setY((float)Context.FrameBufferHeight)
		.setWidth((float)Context.FrameBufferWidth)
		.setHeight(-(float)Context.FrameBufferHeight)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	// Scissor
	vk::Rect2D Scissor;
	Scissor.setOffset({ 0, 0 })
		.setExtent({ Context.FrameBufferWidth, Context.FrameBufferHeight });

	vk::PipelineShaderStageCreateInfo StageCreateInfos[VULKAN_SHADER_MAX_STAGES];
	Memory::Zero(StageCreateInfos, sizeof(vk::PipelineShaderStageCreateInfo) * VULKAN_SHADER_MAX_STAGES);
	for (uint32_t i = 0; i < Config.stage_count; ++i) {
		StageCreateInfos[i] = Stages[i].shader_stage_create_info;
	}

	VulkanPipelineConfig PipelineConfig;
	PipelineConfig.renderpass = Renderpass;
	PipelineConfig.stride = AttributeStride;
	PipelineConfig.attribute_count = (uint32_t)Attributes.size();
	PipelineConfig.attributes = Config.attributes;
	PipelineConfig.descriptor_set_layout_count = Config.descriptor_set_count;
	PipelineConfig.descriptor_set_layout = DescriptorSetLayouts;
	PipelineConfig.stage_count = Config.stage_count;
	PipelineConfig.stages = StageCreateInfos;
	PipelineConfig.viewport = Viewport;
	PipelineConfig.scissor = Scissor;
	PipelineConfig.cull_mode = Config.cull_mode;
	PipelineConfig.is_wireframe = Config.pology_mode == ePology_Mode_Fill ? ePology_Mode_Fill : ePology_Mode_Line;
	PipelineConfig.shaderFlags = Flags;
	PipelineConfig.push_constant_range_count = PushConstantsRangeCount;
	PipelineConfig.push_constant_ranges = PushConstantsRanges;

	if (!Pipeline.Create(&Context, PipelineConfig)) {
		GLOG(Log::eError, "Failed to load graphics pipeline for object shader.");
		return false;
	}

	// Grab the UBO alignment requirement from the device.
	RequiredUboAlignment = Context.Device.GetDeviceProperties().limits.minUniformBufferOffsetAlignment;

	// Make sure the UBO is aligned according to device requirements.
	GlobalUboStride = PaddingAligned(GlobalUboSize, RequiredUboAlignment);
	UboStride = PaddingAligned(UboSize, RequiredUboAlignment);

	// Uniform buffer.
	vk::MemoryPropertyFlags DeviceLocalBits = Context.Device.GetIsSupportDeviceLocalHostVisible() ? vk::MemoryPropertyFlagBits::eDeviceLocal : vk::MemoryPropertyFlags();
	// TODO: max count should be configurable, or perhaps long term support of buffer resizing.
	size_t TotalBufferSize = GlobalUboStride + (UboStride * VULKAN_MAX_MATERIAL_COUNT);
	if (!vkRenderer->CreateRenderbuffer(RenderbufferType::eRenderbuffer_Type_Uniform, TotalBufferSize, true, &UniformBuffer)) {
		GLOG(Log::eError, "Vulkan buffer creation failed for object shader.");
		return false;
	}
	vkRenderer->BindRenderbuffer(&UniformBuffer, 0);

	// Allocate space for the global UBO, which should occupy the _stride_ space, _not_ the actual size used.
	if (!vkRenderer->AllocateRenderbuffer(&UniformBuffer, GlobalUboStride, &GlobalUboOffset)) {
		GLOG(Log::eError, "Failed to allocate space for the uniform buffer!");
		return false;
	}

	// Map the entire buffer's memory.
	MappedUniformBufferBlock = vkRenderer->MapMemory(&UniformBuffer, 0, TotalBufferSize/*TotalBufferSize*/);

	// The index of the global descriptor set.
	const uint32_t DESC_SET_INDEX_GLOBAL = 0;
	// The index of the instance descriptor set.
	const uint32_t DESC_SET_INDEX_INSTANCE = 1;

	// Allocate global descriptor sets, one per frame. Global is always the first set.
	vk::DescriptorSetLayout GlobalLayouts[3] = {
		DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
		DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
		DescriptorSetLayouts[DESC_SET_INDEX_GLOBAL]
	};

	vk::DescriptorSetAllocateInfo AllocInfo;
	AllocInfo.setDescriptorPool(DescriptorPool)
		.setDescriptorSetCount(3)
		.setPSetLayouts(GlobalLayouts);
	if (LogicalDevice.allocateDescriptorSets(&AllocInfo, GlobalDescriptorSets)
		!= vk::Result::eSuccess) {
		GLOG(Log::eError, "Allocate descriptor sets failed.");
		return false;
	}

	return true;
}