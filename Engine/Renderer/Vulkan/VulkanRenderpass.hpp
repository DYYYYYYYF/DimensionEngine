#pragma once

#include "Renderer/Interface/IRenderpass.hpp"
#include "vulkan/vulkan.hpp"
#include "Math/MathTypes.hpp"

class VulkanContext;
class VulkanCommandBuffer;

class VulkanRenderPass : public IRenderpass{
public:
	VulkanRenderPass() {}
	virtual ~VulkanRenderPass() {}

public:
	void Create(VulkanContext* context,
		float depth, uint32_t stencil,
		bool has_prev_pass, bool has_next_pass) override;
	void Destroy(VulkanContext* context) override;

	void Begin(VulkanCommandBuffer* command_buffer, RenderTarget* target) override;
	void End(VulkanCommandBuffer* command_buffer) override;

	vk::RenderPass GetRenderPass() { return *(vk::RenderPass*)&Renderpass; }

public:
	void SetW(float w) { RenderArea.z = w; }
	void SetH(float h) { RenderArea.w = h; }

	void SetX(float x) { RenderArea.x = x; }
	void SetY(float y) { RenderArea.y = y; }

private:
	float Depth;
	uint32_t Stencil;

	bool HasPrevPass;
	bool HasNextPass;
	
	VulkanRenderPassState State;
};