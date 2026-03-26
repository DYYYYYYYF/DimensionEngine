#pragma once

#include <unordered_map>
#include <Defines.hpp>
#include "Framework/Classes/CameraActor.h"
#include "Containers/FString.hpp"

class IRenderer;

struct SCameraSystemConfig {
	unsigned short max_camera_count;
};

/** @brief The name of default camera */
#define DEFAULT_CAMERA_NAME "glabol"

class CameraSystem {
public:
	static DAPI CameraSystem& Get();

public:
	bool Initialize(IRenderer* renderer, SCameraSystemConfig config);
	void Shutdown();

	/**
	 * @brief Acquires a pointer to a camera by name.
	 * If one is not found, a new one is created and returned.
	 * Internal reference counter is incremented.
	 * 
	 * @param name The name of the camera to acquire.
	 * @return A pointer to a camera if successful.
	 */
	ACameraActor* Acquire(const FString& name);

	/**
	 * @brief Releases a camera with the given name, Internal reference counter is
	 * decremented. If this reaches 0, the camera is reset, and the reference is
	 * unable by a new camera.
	 * 
	 * @param name The name of the camera to release.
	 */
	void Release(const FString& name);

	/**
	 * @brief Gets a pointer to the default camera.
	 * 
	 * @return A pointer to default camera.
	 */
	DAPI ACameraActor* GetDefault();

private:
	IRenderer* Renderer;
	bool Initialized;

	SCameraSystemConfig Config;
	std::vector<ACameraActor*> Cameras;
	std::unordered_map<FString, uint64_t> CameraMap;
	
	ACameraActor* DefaultCamera;
};

