#pragma once

#include "Renderer/RendererBackend.hpp"
#include "VulkanTypes.hpp"

struct SVulkanContext;

class VulkanBackend : public IRendererBackend {
public:
	VulkanBackend();
	virtual ~VulkanBackend();

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

protected:
	struct SVulkanContext Context;

};