#pragma once

#include "Defines.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include <unordered_map>

class IRenderer;

struct SMaterialSystemConfig {
	uint32_t max_material_count = 512;
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


	/**
	 * @brief Applies global-level data for the material shader id.
	 *
	 * @param shader_id The identifier of the shader to apply globals for.
	 * @param projection A constant pointer to a projection matrix.
	 * @param view A constant pointer to a view matrix.
	 * @return True on success; otherwise false.
	 */
	static bool ApplyGlobal(uint32_t shader_id, size_t renderer_frame_number, const Matrix4& projection, const Matrix4& view, const Vector4& ambient_color, const Vector3& view_position, uint32_t render_mode, float global_time);

	/**
	 * @brief Applies instance-level material data for the given material.
	 *
	 * @param mat A pointer to the material to be applied.
	 * @param need_update Indicates if the material needs to be update.
	 * @return True on success; otherwise false.
	 */
	static bool ApplyInstance(Material* mat, bool need_update);

	/**
	 * @brief Applies local-level material data (typically just model matrix).
	 *
	 * @param m A pointer to the material to be applied.
	 * @param model A constant pointer to the model matrix to be applied.
	 * @return True on success; otherwise false.
	 */
	static bool ApplyLocal(Material* mat, const Matrix4& model);

private:
	static bool CreateDefaultMaterial();

public:
	static SMaterialSystemConfig MaterialSystemConfig;
	static Material* DefaultMaterial;

	// Array of registered materials.
	static std::vector<Material*> RegisteredMaterials;
	// Hashtable for material lookups.
	static std::unordered_map<std::string, uint32_t> MaterialMap;

	// Know locations for the material shader.
	static MaterialShaderUniformLocations MaterialLocations;
	static uint32_t MaterialShaderID;

	// Know locations for the deferred lighting material shader.
	static DRShaderUniformLocations DeferredLightMaterialLocations;
	static uint32_t DeferredLightMaterialShaderID;

	// Know locations for the ui shader.
	static UIShaderUniformLocations UILocations;
	static uint32_t UIShaderID;

	static bool Initilized;

	static IRenderer* Renderer;

};