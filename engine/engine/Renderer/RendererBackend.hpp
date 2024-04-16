#pragma once

#include "RendererTypes.hpp"

struct SPlatformState;

class IRendererBackend {
public:
	IRendererBackend();
	virtual ~IRendererBackend();

public:
	virtual bool Init(RendererBackendType type, struct SPlatformState* plat_state);
	virtual void Destroy();

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) = 0;
	virtual void Shutdown() = 0;

	virtual bool BeginFrame(double delta_time) = 0;
	virtual bool EndFrame(double delta_time) = 0;
	virtual void Resize(unsigned short width, unsigned short height) = 0;

public:
	SPlatformState* GetPlatformState() { return PlatformState; }

	size_t GetFrameNum() const { return FrameNum; }
	void SetFrameNum(size_t num) { FrameNum = num; }

private:
	RendererBackendType BackendType;
	struct SPlatformState* PlatformState;
	size_t FrameNum;

};

