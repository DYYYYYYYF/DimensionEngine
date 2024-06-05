#include "CameraSystem.h"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Containers/TString.hpp"

IRenderer* CameraSystem::Renderer = nullptr;
bool CameraSystem::Initialized = false;
SCameraSystemConfig CameraSystem::Config;
HashTable CameraSystem::Lookup;
void* CameraSystem::HashTableBlock = nullptr;
CameraLookup* CameraSystem::Cameras = nullptr;
Camera CameraSystem::DefaultCamera;

bool CameraSystem::Initialize(IRenderer* renderer, SCameraSystemConfig config) {
	if (config.max_camera_count == 0) {
		LOG_FATAL("Texture system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		LOG_FATAL("Texture system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initialized) {
		return true;
	}

	Config = config;
	Renderer = renderer;

	// Block of memory will block for array, then block for hashtable.
	size_t ArraryRequirement = sizeof(CameraLookup) * Config.max_camera_count;
	size_t HashtableRequirement = sizeof(CameraLookup) * Config.max_camera_count;

	// The array block is after the state. Already allocated, so just set the pointer.
	Cameras = (CameraLookup*)Memory::Allocate(ArraryRequirement, MemoryType::eMemory_Type_Array);

	// Create a hashtable for texture lookups.
	HashTableBlock = (CameraLookup*)Memory::Allocate(HashtableRequirement, MemoryType::eMemory_Type_DArray);
	Lookup.Create(sizeof(CameraLookup), Config.max_camera_count, HashTableBlock, false);

	// Fill the hashtable with invalid references to use as a default.
	unsigned short InvalidID = INVALID_ID_U16;
	Lookup.Fill(&InvalidID);

	// Invalidate all cameras in the array.
	uint32_t Count = Config.max_camera_count;
	for (uint32_t i = 0; i < Count; ++i) {
		Cameras[i].id = INVALID_ID_U16;
		Cameras[i].reference_count = 0;
	}

	// Setup default camera.
	DefaultCamera = Camera();

	Initialized = true;
	return true;
}

void CameraSystem::Shutdown() {

}

Camera* CameraSystem::Acquire(const char* name) {
	if (Initialized) {
		if (StringEquali(name, DEFAULT_CAMERA_NAME)) {
			return &DefaultCamera;
		}

		unsigned short ID = INVALID_ID_U16;
		if (!Lookup.Get(name, &ID)) {
			LOG_ERROR("Camera system Acquire() failed lookup. returned nullptr.");
			return nullptr;
		}

		if (ID == INVALID_ID_U16) {
			// Find free slot
			for (unsigned short i = 0; i < Config.max_camera_count; ++i) {
				if (i == INVALID_ID_U16) {
					ID = i;
					break;
				}
			}

			if (ID == INVALID_ID_U16) {
				LOG_ERROR("Camera system Acquire() failed to acquire new slot. Adjust camera system config to allow more, return nullptr.");
				return nullptr;
			}

			// Create/register the new camera.
			LOG_INFO("Creating new camera named '%s'.", name);
			Cameras[ID].c = Camera();
			Cameras[ID].id = ID;

			// Update the hashtable.
			Lookup.Set(name, &ID);
		}

		Cameras[ID].reference_count++;
		return &Cameras[ID].c;
	}

	LOG_ERROR("Camera system acquire called before system initialization. return nullptr.");
	return nullptr;
}

void CameraSystem::Release(const char* name) {
	if (Initialized) {
		if (StringEquali(name, DEFAULT_CAMERA_NAME)) {
			LOG_WARN("Cannot release default camera. Nothing was done.");
			return;
		}

		unsigned short ID = INVALID_ID_U16;
		if (!Lookup.Get(name, &ID)) {
			LOG_WARN("Camera system release failed lookup. Nothing was done.");
			return;
		}

		if (ID != INVALID_ID_U16) {
			// Decrement the reference count, and reset the camera if the counter reaches 0.
			Cameras[ID].reference_count--;
			if (Cameras[ID].reference_count < 1) {
				Cameras[ID].c.Reset();
				Cameras[ID].id = INVALID_ID_U16;
				Lookup.Set(name, &Cameras[ID].id);
			}
		}
	}
}

Camera* CameraSystem::GetDefault() {
	if (Initialized) {
		return &DefaultCamera;
	}

	return nullptr;
}


