#include "RendererFrontend.hpp"
#include "RendererBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

IRenderer::IRenderer(const char* application_name, struct SPlatformState* plat_state) : Backend(nullptr){
	if (!Initialize(application_name, plat_state)) {
		return;
	}

}

IRenderer::~IRenderer() {
	Shutdown();
}

bool IRenderer::Initialize(const char* application_name, struct SPlatformState* plat_state) {
	Backend = (IRendererBackend*)Memory::Allocate(sizeof(IRendererBackend), MemoryType::eMemory_Type_Renderer);
	
	// TODO: make this configurable
	Backend->Init(eRenderer_Backend_Type_Vulkan, plat_state);
	Backend->SetFrameNum(0);

	if (!Backend->Initialize(application_name, plat_state)) {
		UL_FATAL("Renderer backend init failed.");
		return false;
	}

	return true;
}

void IRenderer::Shutdown() {
	Backend->Shutdown();
	Memory::Free(Backend, sizeof(IRendererBackend), eMemory_Type_Renderer);
}

void IRenderer::OnResize(unsigned short width, unsigned short height) {
	
}

bool IRenderer::BeginFrame(double delta_time) {
	return Backend->BeginFrame(delta_time);
}

bool IRenderer::EndFrame(double delta_time) {
	bool result = Backend->EndFrame(delta_time);
	Backend->SetFrameNum(Backend->GetFrameNum() + 1);
	return false;
}

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	if (BeginFrame(packet->delta_time)) {
		bool result = EndFrame(packet->delta_time);

		if (!result) {
			UL_ERROR("Renderer end frame failed.");
			return false;
		}

	}

	return true;
}
