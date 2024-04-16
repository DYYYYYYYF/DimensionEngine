#include "RendererBackend.hpp"


IRendererBackend::IRendererBackend() : PlatformState(nullptr), FrameNum(0) {}
IRendererBackend::~IRendererBackend() {}

bool IRendererBackend::Init(RendererBackendType type, struct SPlatformState* plat_state) {
	if (plat_state == nullptr) {
		return false;
	}

	PlatformState = plat_state;
	if (type == eRenderer_Backend_Type_Vulkan) {
		// TODO: fill

		return true;
	}

	return false;
}

void IRendererBackend::Destroy() {

}