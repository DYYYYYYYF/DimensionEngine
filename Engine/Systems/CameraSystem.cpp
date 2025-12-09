#include "CameraSystem.h"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Containers/TString.hpp"

IRenderer* CameraSystem::Renderer = nullptr;
bool CameraSystem::Initialized = false;
SCameraSystemConfig CameraSystem::Config;
Camera* CameraSystem::DefaultCamera = nullptr;
std::vector<Camera*> CameraSystem::Cameras;
std::unordered_map<std::string, uint16_t> CameraSystem::CameraMap;

bool CameraSystem::Initialize(IRenderer* renderer, SCameraSystemConfig config) {
	if (config.max_camera_count == 0) {
		GLOG(Log::eFatal, "Texture system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		GLOG(Log::eFatal, "Texture system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initialized) {
		return true;
	}

	Config = config;
	Renderer = renderer;
	Cameras.resize(Config.max_camera_count);

	// Setup default camera.
	DefaultCamera = NewObject<Camera>(0);
	Cameras[0] = DefaultCamera;
	CameraMap[DEFAULT_CAMERA_NAME] = 0;

	Initialized = true;
	return true;
}

void CameraSystem::Shutdown() {
	for (Camera* c : Cameras) {
		if (c) {
			DeleteObject(c);
			c = nullptr;
		}
	}

	Cameras.clear();
	std::vector<Camera*>().swap(Cameras);
}

Camera* CameraSystem::Acquire(const char* name) {
	if (Initialized) {
		if (StringEquali(name, DEFAULT_CAMERA_NAME)) {
			return DefaultCamera;
		}

		uint16_t ID = INVALID_ID_U16;
		if (CameraMap.find(name) == CameraMap.end()) {
			GLOG(Log::eError, "Camera system Acquire() failed lookup. returned nullptr.");
			return nullptr;
		}

		ID = CameraMap[name];
		if (ID == INVALID_ID_U16) {
			// Find free slot
			for (uint16_t i = 0; i < Config.max_camera_count; ++i) {
				if (Cameras[i] == nullptr || Cameras[i]->GetID() == INVALID_ID_U16) {
					ID = i;
					break;
				}
			}

			if (ID == INVALID_ID_U16) {
				GLOG(Log::eError, "Camera system Acquire() failed to acquire new slot. Adjust camera system config to allow more, return nullptr.");
				return nullptr;
			}

			// Create/register the new camera.
			GLOG(Log::eInfo, "Creating new camera named '%s'.", name);
			Camera* NewCamera = NewObject<Camera>(ID);
			if (NewCamera == nullptr) {
				GLOG(Log::eError, "Create camera %s failed.", name);
				return nullptr;
			}

			// Update the hashtable.
			CameraMap[name] = ID;
		}

		Cameras[ID]->IncreaseReferenceCount();
		return Cameras[ID];
	}

	GLOG(Log::eError, "Camera system acquire called before system initialization. return nullptr.");
	return nullptr;
}

void CameraSystem::Release(const char* name) {
	if (Initialized) {
		if (StringEquali(name, DEFAULT_CAMERA_NAME)) {
			GLOG(Log::eWarn, "Cannot release default camera. Nothing was done.");
			return;
		}

		uint16_t ID = INVALID_ID_U16;
		if (CameraMap.find(name) == CameraMap.end()) {
			GLOG(Log::eWarn, "Camera system release failed lookup. Nothing was done.");
			return;
		}

		ID = CameraMap[name];
		if (ID != INVALID_ID_U16) {
			// Decrement the reference count, and reset the camera if the counter reaches 0.
			Camera* Cam = Cameras[ID];
			if (Cam == nullptr) {
				GLOG(Log::eFatal, "Invalid camera refer. It should not happened.");
				return;
			}

			Cam->DecreaseReferenceCount();
			if (Cam->GetReferenceCount() < 1) {
				Cam->Reset();
				Cam->SetID(INVALID_ID_U16);
				CameraMap[name] = INVALID_ID_U16;
			}
		}
	}
}

Camera* CameraSystem::GetDefault() {
	if (Initialized && DefaultCamera) {
		return DefaultCamera;
	}

	return nullptr;
}


