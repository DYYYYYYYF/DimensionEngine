#include "MaterialSystem.h"

#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Math/MathTypes.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Systems/TextureSystem.h"

// TODO: temp
#include "Platform/FileSystem.hpp"

SMaterialSystemConfig MaterialSystem::MaterialSystemConfig;
Material MaterialSystem::DefaultMaterial;
Material* MaterialSystem::RegisteredMaterials = nullptr;
SMaterialReference* MaterialSystem::TableMemory = nullptr;
HashTable MaterialSystem::RegisteredMaterialTable;
bool MaterialSystem::Initilized = false;
IRenderer* MaterialSystem::Renderer = nullptr;

bool MaterialSystem::Initialize(IRenderer* renderer, SMaterialSystemConfig config) {
	if (config.max_material_count == 0) {
		UL_FATAL("Material system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		UL_FATAL("Material system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initilized) {
		return true;
	}

	MaterialSystemConfig = config;
	Renderer = renderer;

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
	}

	// Create default textures for use in the system.
	if (!CreateDefaultMaterial()) {
		UL_FATAL("Create default material failed. Application quit now!");
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

	// Destroy default material.
	DestroyDefaultMaterial();
}

Material* MaterialSystem::Acquire(const char* name) {
	// Load the given material configuration from disk.
	SMaterialConfig MaterialConfig;
	Memory::Zero(MaterialConfig.name, sizeof(char) * MATERIAL_NAME_MAX_LENGTH);
	Memory::Zero(MaterialConfig.diffuse_map_name, sizeof(char) * TEXTURE_NAME_MAX_LENGTH);

	// Load file from disk.
	// TODO: Should be able to be located anywhere.
	char* FormatStr = "../Asset/Materials/%s.%s";
	char FullFilePath[512];

	// TODO: Try different extensions.
	sprintf_s(FullFilePath, FormatStr, name, "dmt");
	if (!LoadConfigurationFile(FullFilePath, &MaterialConfig)) {
		UL_ERROR("Failed to load material file: '%s'. nullptr will be returned.", FullFilePath);
		return nullptr;
	}

	// Now acquire from loaded config.
	return AcquireFromConfig(MaterialConfig);
}

Material* MaterialSystem::AcquireFromConfig(SMaterialConfig config) {
	// Return default material.
	if (strcmp(config.name, DEFAULT_MATERIAL_NAME) == 0) {
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
				UL_FATAL("Material acquire failed. Material system cannot hold anymore materials. Adjust configuration to allow more.");
				return nullptr;
			}

			// Create new material.
			if (!LoadMaterial(config, m)) {
				UL_ERROR("Load %s material failed.", config.name);
				return nullptr;
			}

			if (m->Generation == INVALID_ID) {
				m->Generation = 0;
			}
			else {
				m->Generation++;
			}

			// Also use the handle as the material id.
			m->Id = Ref.handle;
			UL_INFO("Material '%s' does not yet exist. Created and RefCount is now %i.", config.name, Ref.reference_count);
		}
		else {
			UL_INFO("Material '%s' already exist. RefCount increased to %i.", config.name, Ref.reference_count);
		}

		// Update the entry.
		RegisteredMaterialTable.Set(config.name, &Ref);
		return &RegisteredMaterials[Ref.handle];
	}

	// NOTO: This can only happen in the event something went wrong with the state.
	UL_ERROR("Material acquire failed to acquire material % s, nullptr will be returned.", config.name);
	return nullptr;
}

void MaterialSystem::Release(const char* name) {
	// Ignore release requests for the default material.
	if (strcmp(name, DEFAULT_MATERIAL_NAME)) {
		return;
	}

	SMaterialReference Ref;
	if (RegisteredMaterialTable.Get(name, &Ref)) {
		if (Ref.reference_count == 0) {
			UL_WARN("Tried to release non-existent material: %s", name);
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
			UL_INFO("Released material '%s'. Material unloaded.", name);
		}
		else {
			UL_INFO("Release material '%s'. Now has a reference count of '%i' (auto release = %s)", name,
				Ref.reference_count, Ref.auto_release ? "True" : "False");
		}

		// Update the entry.
		RegisteredMaterialTable.Set(name, &Ref);
	}
	else {
		UL_ERROR("Material release failed to release material '%s'.", name);
	}
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

	// Diffuse color
	mat->DiffuseColor = config.diffuse_color;

	// Diffuse map
	if (strlen(config.diffuse_map_name) > 0) {
		mat->DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
		mat->DiffuseMap.texture = TextureSystem::Acquire(config.diffuse_map_name, true);
		if (mat->DiffuseMap.texture == nullptr) {
			UL_WARN("Unable to load texture '%s' for material '%s', using default.", config.diffuse_map_name, mat->Name);
			mat->DiffuseMap.texture = TextureSystem::GetDefaultTexture();
		}
	}
	else {
		// NOTE: Only set for clarity, as call to Memory::Zero above does this already.
		mat->DiffuseMap.usage = TextureUsage::eTexture_Usage_Unknown;
		mat->DiffuseMap.texture = nullptr;
	}

	// TODO: other maps.

	// Send it off to the renderer to acquire resources.
	if (!Renderer->CreateMaterial(mat)) {
		UL_ERROR("Failed to acquire renderer resources for material '%s'.", mat->Name);
		return false;
	}

	return true;
}

void MaterialSystem::DestroyMaterial(Material* mat) {
	UL_INFO("Destroying material '%s'...", mat->Name);

	// Release texture references.
	if (mat->DiffuseMap.texture != nullptr) {
		TextureSystem::Release(mat->DiffuseMap.texture->Name);
	}

	//Release renderer resources.
	Renderer->DestroyMaterial(mat);

	// Zero it out, invalidate Ids.
	Memory::Zero(mat, sizeof(Material));
	mat->Id = INVALID_ID;
	mat->Generation = INVALID_ID;
	mat->InternalId = INVALID_ID;
}

bool MaterialSystem::CreateDefaultMaterial() {
	Memory::Zero(&DefaultMaterial, sizeof(Material));
	DefaultMaterial.Id = INVALID_ID;
	DefaultMaterial.Generation = INVALID_ID;
	strncmp(DefaultMaterial.Name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
	DefaultMaterial.DiffuseColor = Vec4{1.0f, 1.0f, 1.0f, 1.0f};
	DefaultMaterial.DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	DefaultMaterial.DiffuseMap.texture = TextureSystem::GetDefaultTexture();

	if (!Renderer->CreateMaterial(&DefaultMaterial)) {
		UL_ERROR("Create default material failed. Application quit now!");
		return false;
	}

	return true;
}

void MaterialSystem::DestroyDefaultMaterial() {

}

bool MaterialSystem::LoadConfigurationFile(const char* path, SMaterialConfig* config) {
	FileHandle f;
	if (!FileSystemOpen(path, FileMode::eFile_Mode_Read, false, &f)) {
		UL_ERROR("Load configuration file - unable to open material file for reading: '%s'.", path);
		return false;
	}

	// Read each line of the file.
	char LineBuffer[512] = "";
	char* p = &LineBuffer[0];
	size_t LineLength = 0;
	uint32_t LineNumber = 1;
	while (FileSystemReadLine(&f, 511, &p, &LineLength)) {
		// Trim the string.
		char* Trimmed = Strtrim(LineBuffer);

		// Get the trimmed length.
		LineLength = strlen(Trimmed);

		// Skip blank lines and comments.
		if (LineLength < 1 || Trimmed[0] == '#') {
			LineNumber++;
			continue;
		}

		// Split into var-value
		int EqualIndex = StringIndexOf(Trimmed, '=');
		if (EqualIndex == -1) {
			UL_WARN("Potential formatting issue found in file '%s': '=' token not found. Skiping line %ui.", path, LineNumber);
			LineNumber++;
			continue;
		}

		// Assume a max of 64 characters for the variable name.
		char RawVarName[64];
		Memory::Zero(RawVarName, sizeof(char) * 64);
		StringMid(RawVarName, Trimmed, 0, EqualIndex);
		char* TrimmedVarName = Strtrim(RawVarName);

		// Assume a max of 511-64(446) characters for the max length of the value to account for the variable name and the '='.
		char RawValue[446];
		Memory::Zero(RawValue, sizeof(char) * 446);
		StringMid(RawValue, Trimmed, EqualIndex + 1);
		char* TrimmedValue = Strtrim(RawValue);

		// Process the variable.
		if (strcmp(TrimmedVarName, "version") == 0) {
			//TODO: version

		}
		else if (strcmp(TrimmedVarName, "name") == 0) {
			strncpy(config->name, TrimmedValue, MATERIAL_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "diffuse_map_name") == 0) {
			strncpy(config->diffuse_map_name, TrimmedValue, TEXTURE_NAME_MAX_LENGTH);
		}
		else if (strcmp(TrimmedVarName, "diffuse_color") == 0) {
			// Parse the color
			config->diffuse_color = Vec4::StringToVec4(TrimmedValue);
		}

		// TODO: more fields.

		// Clear the line buffer.
		Memory::Zero(LineBuffer, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(&f);
	return true;
}
