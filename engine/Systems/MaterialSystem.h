#pragma once

#include "Defines.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Containers/THashTable.hpp"

#define DEFAULT_MATERIAL_NAME "default"

class IRenderer;

struct SMaterialSystemConfig {
	uint32_t max_material_count;
};

struct SMaterialConfig {
	char name[MATERIAL_NAME_MAX_LENGTH];
	bool auto_release;
	Vec4 diffuse_color;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
};

struct SMaterialReference {
	size_t reference_count;
	uint32_t handle;
	bool auto_release;
};

class MaterialSystem {
public:
	static bool Initialize(IRenderer* renderer, SMaterialSystemConfig config);
	static void Shutdown();

	static Material* Acquire(const char* name);
	static Material* AcquireFromConfig(SMaterialConfig config);

	static void Release(const char* name);

	static Material* GetDefaultMaterial();

	static bool LoadMaterial(SMaterialConfig config, Material* mat);
	static void DestroyMaterial(Material* mat);

private:
	static bool CreateDefaultMaterial();
	static void DestroyDefaultMaterial();
	static bool LoadConfigurationFile(const char* path, SMaterialConfig* config);

public:
	static SMaterialSystemConfig MaterialSystemConfig;
	static Material DefaultMaterial;

	// Array of registered materials.
	static Material* RegisteredMaterials;

	// Hashtable for material lookups.
	static SMaterialReference* TableMemory;
	static HashTable RegisteredMaterialTable;

	static bool Initilized;

	static IRenderer* Renderer;

};