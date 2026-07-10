#pragma once

#include "Defines.hpp"
#include "Containers/FString.hpp"
#include "Rendering/Resources/ResourceTypes.hpp"
#include <unordered_map>

class IRenderer;

struct SMaterialSystemConfig {
	uint32_t max_material_count = 512;
};

class MaterialSystem {
public:
	static MaterialSystem& Get();

public:
	bool Initialize(IRenderer* renderer, SMaterialSystemConfig config);
	void Shutdown();

	Material* Acquire(const FString& name);
	Material* AcquireFromConfig(SMaterialConfig config);

	void Release(const FString& name);

	Material* GetDefaultMaterial();

	bool LoadMaterial(SMaterialConfig config, Material* mat);
	void DestroyMaterial(Material* mat);


	/**
	 * @brief Applies global-level data for the material shader id.
	 *
	 * @param shader_id The identifier of the shader to apply globals for.
	 * @param projection A constant pointer to a projection matrix.
	 * @param view A constant pointer to a view matrix.
	 * @return True on success; otherwise false.
	 */
	bool ApplyGlobal(uint32_t shader_id, size_t renderer_frame_number, const Matrix4& projection, const Matrix4& view, const Vector4& ambient_color, const Vector3& view_position, uint32_t render_mode, float global_time);

	/**
	 * @brief Applies instance-level material data for the given material.
	 *
	 * @param mat A pointer to the material to be applied.
	 * @param need_update Indicates if the material needs to be update.
	 * @return True on success; otherwise false.
	 */
	bool ApplyInstance(Material* mat, bool need_update);

	/**
	 * @brief Applies local-level material data (typically just model matrix).
	 *
	 * @param m A pointer to the material to be applied.
	 * @param model A constant pointer to the model matrix to be applied.
	 * @return True on success; otherwise false.
	 */
	bool ApplyLocal(Material* mat, const Matrix4& model);

private:
	bool CreateDefaultMaterial();
	bool CreateTextureMap(TextureMap& map, TextureUsage usage, const FString& textureName);

	TextureUsage GetTextureUsageFromUniformName(const FString& name) const;

public:
	SMaterialSystemConfig MaterialSystemConfig;
	Material* DefaultMaterial = nullptr;

	// Array of registered materials.
	std::vector<Material*> RegisteredMaterials;
	// Hashtable for material lookups.
	std::unordered_map<FString, uint32_t> MaterialMap;

	// Know locations for the material shader.
	MaterialShaderUniformLocations MaterialLocations;
	uint32_t MaterialShaderID = INVALID_ID;

	// Know locations for the deferred lighting material shader.
	DRShaderUniformLocations DeferredLightMaterialLocations;
	uint32_t DeferredLightMaterialShaderID = INVALID_ID;

	// Know locations for the ui shader.
	UIShaderUniformLocations UILocations;
	uint32_t UIShaderID = INVALID_ID;

	bool Initilized = false;

	IRenderer* Renderer = nullptr;

};