#include "MaterialSystem.h"

#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Math/MathTypes.hpp"
#include "Renderer/RendererFrontend.hpp"

#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"

SMaterialSystemConfig MaterialSystem::MaterialSystemConfig;
Material MaterialSystem::DefaultMaterial;
Material* MaterialSystem::RegisteredMaterials = nullptr;
SMaterialReference* MaterialSystem::TableMemory = nullptr;
HashTable MaterialSystem::RegisteredMaterialTable;
bool MaterialSystem::Initilized = false;
IRenderer* MaterialSystem::Renderer = nullptr;
MaterialShaderUniformLocations MaterialSystem::MaterialLocations;
uint32_t MaterialSystem::MaterialShaderID;
UIShaderUniformLocations MaterialSystem::UILocations;
uint32_t MaterialSystem::UIShaderID;

bool MaterialSystem::Initialize(IRenderer* renderer, SMaterialSystemConfig config) {
	if (config.max_material_count == 0) {
		LOG_FATAL("Material system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		LOG_FATAL("Material system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initilized) {
		return true;
	}

	MaterialSystemConfig = config;
	Renderer = renderer;

	MaterialShaderID = INVALID_ID;
	MaterialLocations.projection = INVALID_ID_U16;
	MaterialLocations.view = INVALID_ID_U16;
	MaterialLocations.ambient_color= INVALID_ID_U16;
	MaterialLocations.diffuse_color= INVALID_ID_U16;
	MaterialLocations.diffuse_texture = INVALID_ID_U16;
	MaterialLocations.normal_texture = INVALID_ID_U16;
	MaterialLocations.specular_texture = INVALID_ID_U16;
	MaterialLocations.shininess = INVALID_ID_U16;
	MaterialLocations.view_position = INVALID_ID_U16;
	MaterialLocations.model = INVALID_ID_U16;
	MaterialLocations.render_mode = INVALID_ID_U16;

	UIShaderID = INVALID_ID;
	UILocations.projection = INVALID_ID_U16;
	UILocations.view = INVALID_ID_U16;
	UILocations.diffuse_color = INVALID_ID_U16;
	UILocations.diffuse_texture = INVALID_ID_U16;
	UILocations.model = INVALID_ID_U16;

	// Block of memory will block for array, then block for hashtable.
	size_t ArraryRequirement = sizeof(Material) * MaterialSystemConfig.max_material_count;
	size_t HashtableRequirement = sizeof(SMaterialReference) * MaterialSystemConfig.max_material_count;

	// The array block is after the state. Already allocated, so just set the pointer.
	RegisteredMaterials = (Material*)Memory::Allocate(ArraryRequirement, MemoryType::eMemory_Type_Material_Instance);

	// Create a hashtable for texture lookups.
	TableMemory = (SMaterialReference*)Memory::Allocate(HashtableRequirement, MemoryType::eMemory_Type_DArray);
	RegisteredMaterialTable.Create(sizeof(STextureReference), MaterialSystemConfig.max_material_count, TableMemory, false);

	// Fill the hashtable with invalid references to use as a default.
	SMaterialReference InvalidRef;
	InvalidRef.auto_release = false;
	InvalidRef.handle = INVALID_ID;		// Primary reason for needing default values.
	InvalidRef.reference_count = 0;
	RegisteredMaterialTable.Fill(&InvalidRef);

	// Invalidate all textures in the array.
	uint32_t Count = MaterialSystemConfig.max_material_count;
	for (uint32_t i = 0; i < Count; ++i) {
		RegisteredMaterials[i].Id = INVALID_ID;
		RegisteredMaterials[i].Generation = INVALID_ID;
		RegisteredMaterials[i].InternalId = INVALID_ID;
		RegisteredMaterials[i].RenderFrameNumer = INVALID_ID;
	}

	// Create default textures for use in the system.
	if (!CreateDefaultMaterial()) {
		LOG_FATAL("Create default material failed. Application quit now!");
		return false;
	}

	Initilized = true;
	return true;
}

void MaterialSystem::Shutdown() {
	// Destroy all loaded textures.
	for (uint32_t i = 0; i < MaterialSystemConfig.max_material_count; ++i) {
		Material* m = &RegisteredMaterials[i];
		if (m->Id != INVALID_ID) {
			DestroyMaterial(m);
		}
	}
}

Material* MaterialSystem::Acquire(const char* name) {
	if (StringEquali(name, DEFAULT_MATERIAL_NAME)) {
		return &DefaultMaterial;
	}

	// Load the given material configuration from disk.
	Resource MatResource;
	if (!ResourceSystem::Load(name, eResource_type_Material, nullptr, &MatResource)) {
		LOG_ERROR("Failed to load material resource, returning nullptr.");
		return nullptr;
	}

	// Now acquire from loaded config.
	Material* Mat;
	if (MatResource.Data) {
		Mat = AcquireFromConfig(*(SMaterialConfig*)MatResource.Data);
	}

	// Clean up
	ResourceSystem::Unload(&MatResource);

	if (Mat == nullptr) {
		LOG_ERROR("Failed to load material resource, returning nullptr.");
		return nullptr;
	}

	return Mat;
}

Material* MaterialSystem::AcquireFromConfig(SMaterialConfig config) {
	// Return default material.
	if (StringEquali(config.name, DEFAULT_MATERIAL_NAME)) {
		return &DefaultMaterial;
	}

	SMaterialReference Ref;
	if (RegisteredMaterialTable.Get(config.name, &Ref)) {
		// This can only be changed the first time a material is loaded.
		if (Ref.reference_count == 0) {
			Ref.auto_release = config.auto_release;
		}

		Ref.reference_count++;
		if (Ref.handle == INVALID_ID) {
			// This means no material exists here. Find a free index first.
			uint32_t Count = MaterialSystemConfig.max_material_count;
			Material* m = nullptr;
			for (uint32_t i = 0; i < Count; ++i) {
				if (RegisteredMaterials[i].Id == INVALID_ID) {
					// A free slot has been found. Use it index as the handle.
					Ref.handle = i;
					m = &RegisteredMaterials[i];
					break;
				}
			}

			// Make sure an empty slot was actually found.
			if (m == nullptr || Ref.handle == INVALID_ID) {
				LOG_FATAL("Material acquire failed. Material system cannot hold anymore materials. Adjust configuration to allow more.");
				return nullptr;
			}

			// Create new material.
			if (!LoadMaterial(config, m)) {
				LOG_ERROR("Load %s material failed.", config.name);
				return nullptr;
			}

			// Get the uniform indices.
			Shader* s = ShaderSystem::GetByID(m->ShaderID);
			// Save off the locations for known types for quick lookups.
			if (MaterialShaderID == INVALID_ID && strcmp(config.shader_name, "Shader.Builtin.World") == 0) {
				MaterialShaderID = s->ID;
				MaterialLocations.projection = ShaderSystem::GetUniformIndex(s, "projection");
				MaterialLocations.view = ShaderSystem::GetUniformIndex(s, "view");
				MaterialLocations.ambient_color = ShaderSystem::GetUniformIndex(s, "ambient_color");
				MaterialLocations.diffuse_color = ShaderSystem::GetUniformIndex(s, "diffuse_color");
				MaterialLocations.diffuse_texture = ShaderSystem::GetUniformIndex(s, "diffuse_texture");
				MaterialLocations.specular_texture = ShaderSystem::GetUniformIndex(s, "specular_texture");
				MaterialLocations.normal_texture = ShaderSystem::GetUniformIndex(s, "normal_texture");
				MaterialLocations.view_position = ShaderSystem::GetUniformIndex(s, "view_position");
				MaterialLocations.shininess = ShaderSystem::GetUniformIndex(s, "shininess");
				MaterialLocations.model = ShaderSystem::GetUniformIndex(s, "model");
				MaterialLocations.render_mode = ShaderSystem::GetUniformIndex(s, "mode");
			}
			else if (UIShaderID == INVALID_ID && strcmp(config.shader_name, "Shader.Builtin.UI") == 0) {
				UIShaderID = s->ID;
				UILocations.projection = ShaderSystem::GetUniformIndex(s, "projection");
				UILocations.view = ShaderSystem::GetUniformIndex(s, "view");
				UILocations.diffuse_color = ShaderSystem::GetUniformIndex(s, "diffuse_color");
				UILocations.diffuse_texture = ShaderSystem::GetUniformIndex(s, "diffuse_texture");
				UILocations.model = ShaderSystem::GetUniformIndex(s, "model");
			}


			if (m->Generation == INVALID_ID) {
				m->Generation = 0;
			}
			else {
				m->Generation++;
			}

			// Also use the handle as the material id.
			m->Id = Ref.handle;
			UL_DEBUG("Material '%s' does not yet exist. Created and RefCount is now %i.", config.name, Ref.reference_count);
		}
		else {
			UL_DEBUG("Material '%s' already exist. RefCount increased to %i.", config.name, Ref.reference_count);
		}

		// Update the entry.
		RegisteredMaterialTable.Set(config.name, &Ref);
		return &RegisteredMaterials[Ref.handle];
	}

	// NOTO: This can only happen in the event something went wrong with the state.
	LOG_ERROR("Material acquire failed to acquire material % s, nullptr will be returned.", config.name);
	return nullptr;
}

void MaterialSystem::Release(const char* name) {
	// Ignore release requests for the default material.
	if (strcmp(name, DEFAULT_MATERIAL_NAME) == 0) {
		return;
	}

	// Take a copy of name, it will be zero-out in DestroyMaterial();
	char* CopyMatName = StringCopy(name);

	SMaterialReference Ref;
	if (RegisteredMaterialTable.Get(CopyMatName, &Ref)) {
		if (Ref.reference_count == 0) {
			LOG_WARN("Tried to release non-existent material: %s", CopyMatName);
			return;
		}

		Ref.reference_count--;
		if (Ref.reference_count == 0 && Ref.auto_release) {
			Material* mat = &RegisteredMaterials[Ref.handle];

			// Release material.
			DestroyMaterial(mat);

			// Reset the reference.
			Ref.handle = INVALID_ID;
			Ref.auto_release = false;
			LOG_INFO("Released material '%s'. Material unloaded.", CopyMatName);
		}

		// Update the entry.
		RegisteredMaterialTable.Set(CopyMatName, &Ref);
	}
	else {
		LOG_ERROR("Material release failed to release material '%s'.", CopyMatName);
	}

	Memory::Free(CopyMatName, sizeof(char) * strlen(CopyMatName) + 1, MemoryType::eMemory_Type_String);
	CopyMatName = nullptr;
}

Material* MaterialSystem::GetDefaultMaterial() {
	if (Initilized) {
		return &DefaultMaterial;
	}

	return nullptr;
}

bool MaterialSystem::LoadMaterial(SMaterialConfig config, Material* mat) {
	Memory::Zero(mat, sizeof(Material));

	// name
	strncpy(mat->Name, config.name, MATERIAL_NAME_MAX_LENGTH);

	mat->ShaderID = ShaderSystem::GetID(config.shader_name);

	// Diffuse color
	mat->DiffuseColor = config.diffuse_color;
	mat->Shininess = config.shininess;

	// Diffuse map
	// TODO: Make configurable.
	mat->DiffuseMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->DiffuseMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->DiffuseMap.repeat_u = TextureRepeat::eTexture_Repeat_Repeat;
	mat->DiffuseMap.repeat_v = TextureRepeat::eTexture_Repeat_Repeat;
	mat->DiffuseMap.repeat_w = TextureRepeat::eTexture_Repeat_Repeat;
	if (!Renderer->AcquireTextureMap(&mat->DiffuseMap)) {
		LOG_ERROR("Unable to acquire resources for diffuse texture map.");
		return false;
	}

	if (strlen(config.diffuse_map_name) > 0) {
		mat->DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
		mat->DiffuseMap.texture = TextureSystem::Acquire(config.diffuse_map_name, true);
		if (mat->DiffuseMap.texture == nullptr) {
			LOG_WARN("Unable to load texture '%s' for material '%s', using default.", config.diffuse_map_name, mat->Name);
			mat->DiffuseMap.texture = TextureSystem::GetDefaultDiffuseTexture();
		}
	}
	else {
		// NOTE: Only set for clarity, as call to Memory::Zero above does this already.
		mat->DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
		mat->DiffuseMap.texture = TextureSystem::GetDefaultDiffuseTexture();
	}

	// Specular map
	mat->SpecularMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->SpecularMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->SpecularMap.repeat_u = TextureRepeat::eTexture_Repeat_Repeat;
	mat->SpecularMap.repeat_v = TextureRepeat::eTexture_Repeat_Repeat;
	mat->SpecularMap.repeat_w = TextureRepeat::eTexture_Repeat_Repeat;
	if (!Renderer->AcquireTextureMap(&mat->SpecularMap)) {
		LOG_ERROR("Unable to acquire resources for specular texture map.");
		return false;
	}

	if (strlen(config.specular_map_name) > 0) {
		mat->SpecularMap.usage = TextureUsage::eTexture_Usage_Map_Specular;
		mat->SpecularMap.texture = TextureSystem::Acquire(config.specular_map_name, true);
		if (mat->SpecularMap.texture == nullptr) {
			LOG_WARN("Unable to load texture '%s' for material '%s', using default.", config.specular_map_name, mat->Name);
			mat->SpecularMap.texture = TextureSystem::GetDefaultSpecularTexture();
		}
	}
	else {
		// NOTE: Only set for clarity, as call to Memory::Zero above does this already.
		mat->SpecularMap.usage = TextureUsage::eTexture_Usage_Map_Specular;
		mat->SpecularMap.texture = TextureSystem::GetDefaultSpecularTexture();
	}

	// Normal map
	mat->NormalMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->NormalMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	mat->NormalMap.repeat_u = TextureRepeat::eTexture_Repeat_Repeat;
	mat->NormalMap.repeat_v = TextureRepeat::eTexture_Repeat_Repeat;
	mat->NormalMap.repeat_w = TextureRepeat::eTexture_Repeat_Repeat;
	if (!Renderer->AcquireTextureMap(&mat->NormalMap)) {
		LOG_ERROR("Unable to acquire resources for normal texture map.");
		return false;
	}

	if (strlen(config.normal_map_name) > 0) {
		mat->NormalMap.usage = TextureUsage::eTexture_Usage_Map_Normal;
		mat->NormalMap.texture = TextureSystem::Acquire(config.normal_map_name, true);
		if (mat->NormalMap.texture == nullptr) {
			LOG_WARN("Unable to load texture '%s' for material '%s', using default.", config.normal_map_name, mat->Name);
			mat->NormalMap.texture = TextureSystem::GetDefaultNormalTexture();
		}
	}
	else {
		// NOTE: Only set for clarity, as call to Memory::Zero above does this already.
		mat->NormalMap.usage = TextureUsage::eTexture_Usage_Map_Normal;
		mat->NormalMap.texture = TextureSystem::GetDefaultNormalTexture();
	}
	// TODO: other maps.

	// Send it off to the renderer to acquire resources.
	Shader* s = ShaderSystem::Get(config.shader_name);
	if (s == nullptr) {
		LOG_ERROR("Unable to load material because its shader was not found: '%s'. This is likely a problem with the material asset.", config.shader_name);
		return false;
	}

	// Gather a list of pointers to texture maps.
	std::vector<TextureMap*> Maps = { &mat->DiffuseMap, &mat->SpecularMap, &mat->NormalMap };
	mat->InternalId = Renderer->AcquireInstanceResource(s, Maps);
	if (mat->InternalId == INVALID_ID) {
		LOG_ERROR("Failed to acquire renderer resources for material '%s'.", mat->Name);
		return false;
	}

	return true;
}

void MaterialSystem::DestroyMaterial(Material* mat) {
	LOG_INFO("Destroying material '%s'...", mat->Name);

	// Release texture references.
	if (mat->DiffuseMap.texture != nullptr) {
		TextureSystem::Release(mat->DiffuseMap.texture->Name);
	}

	// Release texture references.
	if (mat->SpecularMap.texture != nullptr) {
		TextureSystem::Release(mat->SpecularMap.texture->Name);
	}

	// Release texture references.
	if (mat->NormalMap.texture != nullptr) {
		TextureSystem::Release(mat->NormalMap.texture->Name);
	}

	// Release texture map resources.
	Renderer->ReleaseTextureMap(&mat->DiffuseMap);
	Renderer->ReleaseTextureMap(&mat->SpecularMap);
	Renderer->ReleaseTextureMap(&mat->NormalMap);

	//Release renderer resources.
	if (mat->ShaderID != INVALID_ID && mat->InternalId != INVALID_ID) {
		Shader* s = ShaderSystem::GetByID(mat->ShaderID);
		Renderer->ReleaseInstanceResource(s, mat->InternalId);
		mat->ShaderID = INVALID_ID;
	}

	// Zero it out, invalidate Ids.
	Memory::Zero(mat, sizeof(Material));
	mat->Id = INVALID_ID;
	mat->Generation = INVALID_ID;
	mat->InternalId = INVALID_ID;
	mat->RenderFrameNumer = INVALID_ID;
}

bool MaterialSystem::CreateDefaultMaterial() {
	Memory::Zero(&DefaultMaterial, sizeof(Material));
	DefaultMaterial.Id = INVALID_ID;
	DefaultMaterial.Generation = INVALID_ID;
	auto res = strncpy(DefaultMaterial.Name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
	DefaultMaterial.DiffuseColor = Vec4{1.0f, 1.0f, 1.0f, 1.0f};
	DefaultMaterial.DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	DefaultMaterial.DiffuseMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.DiffuseMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.DiffuseMap.repeat_u = eTexture_Repeat_Repeat;
	DefaultMaterial.DiffuseMap.repeat_v = eTexture_Repeat_Repeat;
	DefaultMaterial.DiffuseMap.repeat_w = eTexture_Repeat_Repeat;
	DefaultMaterial.DiffuseMap.texture = TextureSystem::GetDefaultDiffuseTexture();

	DefaultMaterial.SpecularMap.usage = TextureUsage::eTexture_Usage_Map_Specular;
	DefaultMaterial.SpecularMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.SpecularMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.SpecularMap.repeat_u = eTexture_Repeat_Repeat;
	DefaultMaterial.SpecularMap.repeat_v = eTexture_Repeat_Repeat;
	DefaultMaterial.SpecularMap.repeat_w = eTexture_Repeat_Repeat;
	DefaultMaterial.SpecularMap.texture = TextureSystem::GetDefaultSpecularTexture();

	DefaultMaterial.NormalMap.usage = TextureUsage::eTexture_Usage_Map_Normal;
	DefaultMaterial.NormalMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.NormalMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	DefaultMaterial.NormalMap.repeat_u = eTexture_Repeat_Repeat;
	DefaultMaterial.NormalMap.repeat_v = eTexture_Repeat_Repeat;
	DefaultMaterial.NormalMap.repeat_w = eTexture_Repeat_Repeat;
	DefaultMaterial.NormalMap.texture = TextureSystem::GetDefaultNormalTexture();

	std::vector<TextureMap*> Maps = { &DefaultMaterial.DiffuseMap, &DefaultMaterial.SpecularMap, &DefaultMaterial.NormalMap };

	Shader* s = ShaderSystem::Get("Shader.Builtin.World");
	if (s == nullptr) {
		LOG_FATAL("Shader.Builtin.World shader is nullptr.");
		ASSERT(s);
		return false;
	}

	DefaultMaterial.InternalId = Renderer->AcquireInstanceResource(s, Maps);
	if (DefaultMaterial.InternalId == INVALID_ID) {
		LOG_ERROR("Create default material failed. Application quit now!");
		return false;
	}

	// Make sure to assign the shader id.
	DefaultMaterial.ShaderID = s->ID;

	return true;
}

#define MATERIAL_APPLY_OR_FAIL(expr)                  \
    if (!expr) {                                      \
        UL_ERROR("Failed to apply material: %s", expr); \
        return false;                                 \
    }

bool MaterialSystem::ApplyGlobal(uint32_t shader_id, size_t renderer_frame_number, const Matrix4& projection, const Matrix4& view, const Vec4& ambient_color, const Vec3& view_position, uint32_t render_mode) {
	Shader* s = ShaderSystem::GetByID(shader_id);
	if (s == nullptr) {
		return false;
	}

	if (s->RenderFrameNumber == renderer_frame_number) {
		return true;
	}

	if (shader_id == MaterialShaderID) {
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.projection, &projection));
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.view, &view));
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.ambient_color, &ambient_color));
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.view_position, &view_position));
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.render_mode, &render_mode));
	}
	else if (shader_id == UIShaderID) {
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(UILocations.projection, &projection));
		MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(UILocations.view, &view));
	}
	else {
		LOG_ERROR("material_system_apply_global(): Unrecognized shader id '%d' ", shader_id);
		return false;
	}

	MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyGlobal());

	// Sync
	s->RenderFrameNumber = renderer_frame_number;

	return true;
}

bool MaterialSystem::ApplyInstance(Material* mat, bool need_update) {
	// Apply instance-level uniforms.
	MATERIAL_APPLY_OR_FAIL(ShaderSystem::BindInstance(mat->InternalId));
	if (need_update) {
		if (mat->ShaderID == MaterialShaderID) {
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.diffuse_color, &mat->DiffuseColor));
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.diffuse_texture, &mat->DiffuseMap));
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.specular_texture, &mat->SpecularMap));
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.normal_texture, &mat->NormalMap));
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(MaterialLocations.shininess, &mat->Shininess));
		}
		else if (mat->ShaderID == UIShaderID) {
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(UILocations.diffuse_color, &mat->DiffuseColor));
			MATERIAL_APPLY_OR_FAIL(ShaderSystem::SetUniformByIndex(UILocations.diffuse_texture, &mat->DiffuseMap));
		}
		else {
			LOG_ERROR("material_system_apply_instance(): Unrecognized shader id '%d' on shader '%s'.", mat->ShaderID, mat->Name);
			return false;
		}
	}

	MATERIAL_APPLY_OR_FAIL(ShaderSystem::ApplyInstance(need_update));
	return true;
}

bool MaterialSystem::ApplyLocal(Material* mat, const Matrix4& model) {
	if (mat->ShaderID == MaterialShaderID) {
		return ShaderSystem::SetUniformByIndex(MaterialLocations.model, &model);
	}
	else if (mat->ShaderID == UIShaderID) {
		return ShaderSystem::SetUniformByIndex(UILocations.model, &model);
	}

	LOG_ERROR("Unrecognized shader id '%d'", mat->ShaderID);
	return false;
}
