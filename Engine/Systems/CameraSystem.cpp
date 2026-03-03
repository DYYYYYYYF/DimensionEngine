#include "CameraSystem.h"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Containers/TString.hpp"

IRenderer* CameraSystem::Renderer = nullptr;
bool CameraSystem::Initialized = false;
SCameraSystemConfig CameraSystem::Config;
ACameraActor* CameraSystem::DefaultCamera = nullptr;
std::vector<ACameraActor*> CameraSystem::Cameras;
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
	DefaultCamera = NewObject<ACameraActor>();
	Cameras[0] = DefaultCamera;
	CameraMap[DEFAULT_CAMERA_NAME] = 0;

	Initialized = true;
	return true;
}

void CameraSystem::Shutdown() {
	for (ACameraActor* c : Cameras) {
		if (c) {
			DeleteObject(c);
			c = nullptr;
		}
	}

	Cameras.clear();
	std::vector<ACameraActor*>().swap(Cameras);
}

ACameraActor* CameraSystem::Acquire(const std::string& name) {
	if (Initialized) {
		if (StringEquali(name.c_str(), DEFAULT_CAMERA_NAME)) {
			return DefaultCamera;
		}

		uint16_t ID = INVALID_ID;
		if (CameraMap.find(name) == CameraMap.end()) {
			GLOG(Log::eError, "Camera system Acquire() failed lookup. returned nullptr.");
			return nullptr;
		}

		ID = CameraMap[name];
		if (ID != INVALID_ID) {
			Cameras[ID]->IncreaseReferenceCount();
			return Cameras[ID];
		}

		// Create/register the new camera.
		GLOG(Log::eInfo, "Creating new camera named '%s'.", name);
		ACameraActor* NewCamera = NewObject<ACameraActor>();
		ID = NewCamera->GetUniqueID();
		if (NewCamera == nullptr || ID == INVALID_ID) {
			GLOG(Log::eError, "Create camera %s failed.", name);
			return nullptr;
		}

		ASSERT(!Cameras[ID])

		// Update the hashtable.
		CameraMap[name] = ID;
		Cameras[ID]->IncreaseReferenceCount();
		return Cameras[ID];
	}

	GLOG(Log::eError, "Camera system acquire called before system initialization. return nullptr.");
	return nullptr;
}

void CameraSystem::Release(const std::string& name) {
	if (Initialized) {
		if (StringEquali(name.c_str(), DEFAULT_CAMERA_NAME)) {
			GLOG(Log::eWarn, "Cannot release default camera. Nothing was done.");
			return;
		}

		uint16_t ID = INVALID_ID;
		if (CameraMap.find(name) == CameraMap.end()) {
			GLOG(Log::eWarn, "Camera system release failed lookup. Nothing was done.");
			return;
		}

		ID = CameraMap[name];
		if (ID != INVALID_ID) {
			// Decrement the reference count, and reset the camera if the counter reaches 0.
			ACameraActor* Cam = Cameras[ID];
			if (Cam == nullptr) {
				GLOG(Log::eFatal, "Invalid camera refer. It should not happened.");
				return;
			}

			Cam->DecreaseReferenceCount();
			if (Cam->GetReferenceCount() < 1) {
				Cam->Reset();
				CameraMap[name] = INVALID_ID;
			}
		}
	}
}

ACameraActor* CameraSystem::GetDefault() {
	if (Initialized && DefaultCamera) {
		return DefaultCamera;
	}

	return nullptr;
}


