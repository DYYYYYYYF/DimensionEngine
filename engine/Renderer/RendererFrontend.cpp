#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Math/MathTypes.hpp"

IRenderer::IRenderer() : Backend(nullptr) {}

IRenderer::IRenderer(RendererBackendType type, struct SPlatformState* plat_state) : Backend(nullptr){
	if (plat_state == nullptr) {
		return ;
	}

	BackendType = type;
	if (type == eRenderer_Backend_Type_Vulkan) {
		// TODO: fill
		void* TempBackend = (VulkanBackend*)Memory::Allocate(sizeof(VulkanBackend), MemoryType::eMemory_Type_Renderer);
		Backend = new(TempBackend)VulkanBackend();

		// TODO: make this configurable
		Backend->SetFrameNum(0);

		return ;
	}

}

IRenderer::~IRenderer() {
	Shutdown();
}

bool IRenderer::Initialize(const char* application_name, struct SPlatformState* plat_state) {
	if (Backend == nullptr) {
		return false;
	}

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
	if (Backend != nullptr) {
		Backend->Resize(width, height);
	}
	else {
		UL_WARN("Renderer backend does not exist to accept resize: %i %i", width, height);
	}
}

bool IRenderer::BeginFrame(double delta_time) {
	return Backend->BeginFrame(delta_time);
}

bool IRenderer::EndFrame(double delta_time) {
	bool result = Backend->EndFrame(delta_time);
	Backend->SetFrameNum(Backend->GetFrameNum() + 1);
	return result;
}

static float r = -10.0;

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	if (BeginFrame(packet->delta_time)) {

		Matrix4 Projection = Matrix4::Perspective(Deg2Rad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
		Matrix4 View = Matrix4::Identity();
		View.SetTranslation(Vec3{ 0, 0, -10.f });

		r += 0.1f;
		Quaternion Quat = GenerateFromAxisAngle(Vec3{ 0, 0, 1.0f }, r, false);
		Matrix4 Model = QuatToMatrix(Quat);

		// Update UBO buffer
		Backend->UpdateGlobalState(Projection, View, Vec3(), Vec4(1.0f, 1.0f, 1.0f, 1.0f), 0);

		bool result = EndFrame(packet->delta_time);

		if (!result) {
			UL_ERROR("Renderer end frame failed.");
			return false;
		}

	}

	return true;
}
