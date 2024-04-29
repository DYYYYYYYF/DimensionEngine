#include "RendererFrontend.hpp"
#include "Vulkan/VulkanBackend.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include "Math/MathTypes.hpp"

// TODO: temp
#include "Core/Event.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

	Backend->DefaultDiffuse = &DefaultTexture;

	NearClip = 0.01f;
	FarClip = 1000.0f;
	Projection = Matrix4::Perspective(Deg2Rad(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);


	View = Matrix4::Identity();
	View.SetTranslation(Vec3{ 0.0f, 0.0f, -5.0f });

	// NOTE: create default texture, a 256x256 blue/white checkerboard pattern.
	// This is done in code to eliminate asset dependencies.
	UL_INFO("Createing default texture...");
	const uint32_t TexDimension = 256;
	const uint32_t bpp = 4;
	const uint32_t PixelCount = TexDimension * TexDimension;
	unsigned char Pixels[PixelCount * bpp];

	Memory::Set(Pixels, 255, sizeof(unsigned char) * PixelCount * bpp);

	// Each pixel.
	for (size_t row = 0; row < TexDimension; row++) {
		for (size_t col = 0; col < TexDimension; col++) {
			size_t Index = (row * TexDimension) + col;
			size_t IndexBpp = Index * bpp;
			if (row % 2) {
				if (col % 2) {
					Pixels[IndexBpp + 0] = 0;
					Pixels[IndexBpp + 1] = 0;
				}
			}
			else {
				if (!(col % 2)) {
					Pixels[IndexBpp + 0] = 0;
					Pixels[IndexBpp + 1] = 0;
				}
			}
		}
	}

	CreateTexture("Default", false, TexDimension, TexDimension, 4, Pixels, false, &DefaultTexture);
	UL_INFO("Default texture created.");

	// Manually set the texture generation to invalid since this is a default texture.
	DefaultTexture.Generation = INVALID_ID;

	// TODO: Load other texture.
	CreateTexture(&TestDiffuse);

	// Temp
	LoadTexture("Wood", &TestDiffuse);

	return true;
}

void IRenderer::Shutdown() {
	if (Backend != nullptr) {
		DestroyTexture(&DefaultTexture);
		DestroyTexture(&TestDiffuse);

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
		RenderData.textures[0] = &TestDiffuse;
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

bool IRenderer::LoadTexture(const char* name, Texture* texture) {
	// TODO: Should be able to be located anywhere.
	char* FormatStr = "../Asset/Textures/%s.%s";
	const int RequiredChannelCount = 4;
	stbi_set_flip_vertically_on_load(true);
	char FullFilePath[512];

	// TODO: Try different extensions.
	sprintf_s(FullFilePath, FormatStr, name, "png");

	// Use a temporary texture to load into.
	Texture TempTexture;

	unsigned char* data = stbi_load(FullFilePath, (int*)&TempTexture.Width, (int*)&TempTexture.Height,
		(int*)&TempTexture.ChannelCount, RequiredChannelCount);

	TempTexture.ChannelCount = RequiredChannelCount;

	if (data != nullptr) {
		uint32_t CurrentGeneration = texture->Generation;
		texture->Generation = INVALID_ID;

		size_t TotalSize = TempTexture.Width * TempTexture.Height * RequiredChannelCount;
		// Check for transparency.
		bool HasTransparency = false;
		for (size_t i = 0; i < TotalSize; ++i) {
			unsigned char a = data[i + 3];
			if (a < 255) {
				HasTransparency = true;
				break;
			}
		}

		if (stbi_failure_reason() != nullptr) {
			UL_WARN("Load texture failed to load file %s : %s", FullFilePath, stbi_failure_reason());
		}

		//Acquire internal texture resources and upload to GPU.
		CreateTexture(name, true, TempTexture.Width, TempTexture.Height, TempTexture.ChannelCount, data, HasTransparency, &TempTexture);

		// Take a copy of the old texture.
		Texture Old = *texture;

		// Assign the temp texture to the pointer.
		*texture = TempTexture;

		// Destroy the old texture.
		DestroyTexture(&Old);

		if (CurrentGeneration == INVALID_ID) {
			texture->Generation = 0;
		}
		else {
			texture->Generation = CurrentGeneration + 1;
		}

		// Clean up data.
		stbi_image_free(data);
		return true;
	}
	else {
		if (stbi_failure_reason() != nullptr) {
			UL_WARN("Load texture failed to load file %s : %s", FullFilePath, stbi_failure_reason());
		}

		return false;
	}
}

void IRenderer::CreateTexture(const char* name, bool auto_release, int width, int height, int channel_count,
	const unsigned char* pixels, bool has_transparency, Texture* texture) {
	Backend->CreateTexture(name, auto_release, width, height, channel_count, pixels, has_transparency, texture);
}

void IRenderer::DestroyTexture(Texture* txture) {
	Backend->DestroyTexture(txture);
}