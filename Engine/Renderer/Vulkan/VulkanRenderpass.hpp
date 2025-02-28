#pragma once

#include "Renderer/Interface/IRenderpass.hpp"
#include "vulkan/vulkan.hpp"
#include "Math/MathTypes.hpp"

class VulkanContext;
class VulkanCommandBuffer;

class VulkanRenderPass : public IRenderpass{
public:
	virtual bool Create(VulkanContext* context, const RenderpassConfig& config) override;
	virtual void Destroy() override;

	virtual void Begin(RenderTarget* target) override;
	virtual void End() override;

	vk::RenderPass GetRenderPass() { return *(vk::RenderPass*)&Renderpass; }

public:
	void SetW(float w) { RenderArea.z = w; }
	void SetH(float h) { RenderArea.w = h; }

	void SetX(float x) { RenderArea.x = x; }
	void SetY(float y) { RenderArea.y = y; }

private:
	VulkanContext* Context = nullptr;

	float Depth;
	uint32_t Stencil;

	VulkanRenderPassState State;
};