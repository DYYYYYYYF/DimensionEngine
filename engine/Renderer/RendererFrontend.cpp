#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Math/MathTypes.hpp"
#include "Systems/TextureSystem.h"

// TODO: temp
#include "Core/Event.hpp"

// TODO: temp

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

	NearClip = 0.01f;
	FarClip = 1000.0f;
	Projection = Matrix4::Perspective(Deg2Rad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);


	View = Matrix4::Identity();
	View.SetTranslation(Vec3{ 0.0f, 0.0f, -5.0f });

	TestDiffuse = nullptr;

	return true;
}

void IRenderer::Shutdown() {
	if (Backend != nullptr) {

		Backend->Shutdown();
		Memory::Free(Backend, sizeof(IRendererBackend), eMemory_Type_Renderer);
	}

	Backend = nullptr;
}

void IRenderer::OnResize(unsigned short width, unsigned short height) {
	if (Backend != nullptr) {
		Projection = Matrix4::Perspective(Deg2Rad(45.0f), (float)width / (float)height, NearClip, FarClip);
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

static float x = 0.0f;

bool IRenderer::DrawFrame(SRenderPacket* packet) {
	if (BeginFrame(packet->delta_time)) {
		// Update UBO buffer
		Backend->UpdateGlobalState(Projection, View, Vec3(0.0f, 0.0f, 0.0f), Vec4(1.0f, 1.0f, 1.0f, 1.0f), 0);

		Quaternion Quat = QuaternionFromAxisAngle(Vec3{ 0.0f, 0.0f, 1.0f }, x, true);
		//Matrix4 Model = QuatToRotationMatrix(Quat, Vec3());
		Matrix4 Model = Matrix4::Identity();

		GeometryRenderData RenderData = {};
		RenderData.object_id = 0;
		RenderData.model = Model;

		// TODO: Temp
		// Grab the default if does not exist.
		if (TestDiffuse == nullptr) {
			TestDiffuse = TextureSystem::GetDefaultTexture();
			//TestDiffuse = TextureSystem::Acquire("Wood", true);

		}
		RenderData.textures[0] = TestDiffuse;

		x += 0.001f;
		Backend->UpdateObject(RenderData);

		bool result = EndFrame(packet->delta_time);
		if (!result) {
			UL_ERROR("Renderer end frame failed.");
			return false;
		}

	}

	return true;
}

void IRenderer::CreateTexture(Texture* texture) {
	Memory::Zero(texture, sizeof(Texture));
	texture->Generation = INVALID_ID;
}

void IRenderer::CreateTexture(const char* name, int width, int height, int channel_count,
	const unsigned char* pixels, bool has_transparency, Texture* texture) {
	Backend->CreateTexture(name, width, height, channel_count, pixels, has_transparency, texture);
}

void IRenderer::DestroyTexture(Texture* txture) {
	Backend->DestroyTexture(txture);
}