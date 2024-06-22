#pragma once

#include "Renderer/Camera.hpp"
#include "Containers/THashTable.hpp"

class IRenderer;

struct SCameraSystemConfig {
	unsigned short max_camera_count;
};

struct CameraLookup {
	unsigned short id;
	unsigned short reference_count;
	Camera c;
};

/** @brief The name of default camera */
#define DEFAULT_CAMERA_NAME "glabol"

class DAPI CameraSystem {
public:
	static bool Initialize(IRenderer* renderer, SCameraSystemConfig config);
	static void Shutdown();

	/**
	 * @brief Acquires a pointer to a camera by name.
	 * If one is not found, a new one is created and returned.
	 * Internal reference counter is incremented.
	 * 
	 * @param name The name of the camera to acquire.
	 * @return A pointer to a camera if successful.
	 */
	static Camera* Acquire(const char* name);

	/**
	 * @brief Releases a camera with the given name, Internal reference counter is
	 * decremented. If this reaches 0, the camera is reset, and the reference is
	 * unable by a new camera.
	 * 
	 * @param name The name of the camera to release.
	 */
	static void Release(const char* name);

	/**
	 * @brief Gets a pointer to the default camera.
	 * 
	 * @return A pointer to default camera.
	 */
	static Camera* GetDefault();

private:
	static IRenderer* Renderer;
	static bool Initialized;

	static SCameraSystemConfig Config;
	static HashTable Lookup;
	static void* HashTableBlock;
	static CameraLookup* Cameras;
	
	static Camera DefaultCamera;
};

