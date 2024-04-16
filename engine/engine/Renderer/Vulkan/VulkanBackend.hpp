#pragma once

#include "Renderer/RendererBackend.hpp"

class VulkanBackend : IRendererBackend {
public:
	VulkanBackend();
	virtual ~VulkanBackend();

public:
	virtual bool Init(RendererBackendType type, struct SPlatformState* plat_state) override;
	virtual void Destroy() override;

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame(double delta_time) override;
	virtual bool EndFrame(double delta_time) override;
	virtual void Resize(unsigned short width, unsigned short height) override;

};