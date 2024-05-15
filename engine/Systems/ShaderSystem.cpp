#include "ShaderSystem.h"

#include "Renderer/RendererFrontend.hpp"
#include "Systems/TextureSystem.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

IRenderer* ShaderSystem::Renderer = nullptr;
SShaderSystemConfig ShaderSystem::Config;
HashTable ShaderSystem::Lookup;
void* ShaderSystem::LookupMemory = nullptr;

uint32_t ShaderSystem::CurrentShaderID;
Shader* ShaderSystem::Shaders;
bool ShaderSystem::Initilized = false;

bool ShaderSystem::Initialize(IRenderer* renderer, SShaderSystemConfig config) {
	if (renderer == nullptr) {
		return false;
	}

	Renderer = renderer;
	if (config.max_shader_count < 512) {
		if (config.max_shader_count == 0) {
			UL_ERROR("shader_system_initialize - config.max_shader_count must be greater than 0");
			return false;
		}
		else {
			// This is to help avoid hashtable collisions.
			UL_ERROR("shader_system_initialize - config.max_shader_count is recommended to be at least 512.");
		}
	}
	
	// Figure out how large of a hashtable is needed.
	// Block of memory will contain state structure then the block for the hashtable.
	LookupMemory = Memory::Allocate(sizeof(uint32_t) * config.max_shader_count, MemoryType::eMemory_Type_Array);
	Shaders = (Shader*)Memory::Allocate(sizeof(Shader) * config.max_shader_count, MemoryType::eMemory_Type_Array);
	Config = config;
	CurrentShaderID = INVALID_ID;

	Lookup.Create(sizeof(uint32_t), config.max_shader_count, LookupMemory, false);

	// Invalidate all shader ids.
	for (uint32_t i = 0; i < config.max_shader_count; ++i) {
		Shaders[i].ID = INVALID_ID;
	}

	// Fill the table with invalid ids.
	uint32_t InvalidFillID = INVALID_ID;
	if (!Lookup.Fill(&InvalidFillID)) {
		UL_ERROR("hashtable_fill failed.");
		return false;
	}

	for (uint32_t i = 0; i < config.max_shader_count; ++i) {
		Shaders[i].ID = INVALID_ID;
	}

	Initilized = true;
	return true;
}

void ShaderSystem::Shutdown() {
	if (Initilized) {
		for (uint32_t i = 0; i < Config.max_shader_count; ++i) {
			Shader* s = &Shaders[i];
			if (s->ID != INVALID_ID) {
				DestroyShader(s);
			}
		}

		Lookup.Destroy();
		Memory::Free(LookupMemory, sizeof(uint32_t) * Config.max_shader_count, MemoryType::eMemory_Type_Array);
		Memory::Free(Shaders, sizeof(Shader) * Config.max_shader_count, MemoryType::eMemory_Type_Array);
	}
}

bool ShaderSystem::Create(ShaderConfig* config) {
	uint32_t ID = NewShaderID();
	Shader* OutShader = &Shaders[ID];
	Memory::Zero(OutShader, sizeof(Shader));
	OutShader->ID = ID;
	if (OutShader->ID = INVALID_ID) {
		UL_ERROR("Unable to find free slot to create new shader. Aborting.");
		return false;
	}
	
	size_t NameLen = strlen(config->name);
	OutShader->Name = (char*)Memory::Allocate(NameLen, MemoryType::eMemory_Type_String);
	strcpy(OutShader->Name, config->name);

	OutShader->State = eShader_State_Not_Created;
	OutShader->UseInstances = config->use_instances;
	OutShader->UseLocals = config->use_local;
	OutShader->PushConstantsRangeCount = 0;
	OutShader->BoundInstanceId = INVALID_ID;
	OutShader->AttributeStride = 0;
	Memory::Zero(OutShader->PushConstantsRanges, sizeof(Range) * 32);

	// Create a hashtable to store uniform array indexes. This provides a direct index into the
	// 'uniforms' array stored in the shader for quick lookups by name.
	size_t ElementSize = sizeof(unsigned short);
	size_t ElementCount = 1024;
	OutShader->HashtableBlock = Memory::Allocate(ElementCount * ElementSize, MemoryType::eMemory_Type_Unknow);
	OutShader->UniformLookup.Create(ElementSize, (uint32_t)ElementCount, OutShader->HashtableBlock, false);

	// Invalidate all spots in the hashtable.
	uint32_t Invalid = INVALID_ID;
	OutShader->UniformLookup.Fill(&Invalid);

	// A running total of the actual global uniform buffer object size.
	OutShader->GlobalUboSize = 0;
	// A running total of the actual instance uniform buffer object size.
	OutShader->UboSize = 0;
	// NOTE: UBO alignment requirement set in renderer backend.

	// This is hard-coded because the Vulkan spec only guarantees that a _minimum_ 128 bytes of space are available,
	// and it's up to the driver to determine how much is available. Therefore, to avoid complexity, only the
	// lowest common denominator of 128B will be used.
	OutShader->PushConstantsStride = 128;
	OutShader->PushConstantsSize = 0;

	unsigned short RenderpassID = Renderer->GetRenderpassID(config->renderpass_name);
	if (RenderpassID == INVALID_ID_U16) {
		UL_ERROR("Unable to find renderpass '%s'", config->renderpass_name);
		return false;
	}

	if (!Renderer->CreateRenderShader(OutShader, RenderpassID, config->stage_cout, config->stage_filenames, config->stages)) {
		UL_ERROR("Error creating shader.");
		return false;
	}

	// Ready to be initialized.
	OutShader->State = eShader_State_Uninitialized;

	// Process attributes.
	for (uint32_t i = 0; i < config->attribute_count; ++i) {
		AddAttribute(OutShader, config->attributes[i]);
	}

	// Process uniforms.
	for (uint32_t i = 0; i < config->uniform_count; ++i) {
		if (config->uniforms[i].type == eShader_Uniform_Type_Sampler) {
			AddSampler(OutShader, config->uniforms[i]);
		}
		else {
			AddUniform(OutShader,  config->uniforms[i]);
		}
	}

	// Initialize the shader.
	if (!Renderer->InitializeRenderShader(OutShader)) {
		UL_ERROR("shader_system_create: initialization failed for shader '%s'.", config->name);
		// NOTE: initialize automatically destroys the shader if it fails.
		return false;
	}

	// At this point, creation is successful, so store the shader id in the hashtable
	// so this can be looked up by name later.
	if (!Lookup.Set(config->name, &OutShader->ID)) {
		// Dangit, we got so far... welp, nuke the shader and boot.
		Renderer->DestroyRenderShader(OutShader);
		return false;
	}

	return true;
}

unsigned ShaderSystem::GetID(const char* shader_name) {
	return GetShaderID(shader_name);
}

Shader* ShaderSystem::GetByID(uint32_t shader_id) {
	if (shader_id >= Config.max_shader_count || Shaders[shader_id].ID == INVALID_ID) {
		return nullptr;
	}

	return &Shaders[shader_id];
}

Shader* ShaderSystem::Get(const char* shader_name) {
	uint32_t ShaderID = GetShaderID(shader_name);
	if (ShaderID == INVALID_ID) {
		return GetByID(ShaderID);
	}

	return nullptr;
}

void ShaderSystem::DestroyShader(Shader* s) {
	Renderer->DestroyRenderShader(s);

	// Set it to be unusable right away.
	s->State = eShader_State_Not_Created;

	// Free the name.
	if (s->Name) {
		size_t Length = strlen(s->Name);
		Memory::Free(s->Name, Length + 1, MemoryType::eMemory_Type_String);
	}

	s->Name = nullptr;
}

void ShaderSystem::Destroy(const char* shader_name) {
	uint32_t ShaderID = GetShaderID(shader_name);
	if (ShaderID == INVALID_ID) {
		return;
	}

	Shader* s = &Shaders[ShaderID];
	DestroyShader(s);
}

bool ShaderSystem::Use(const char* shader_name) {
	uint32_t NextShaderID = GetShaderID(shader_name);
	if (NextShaderID == INVALID_ID) {
		return false;
	}

	return UseByID(NextShaderID);
}

bool ShaderSystem::UseByID(uint32_t shader_id) {
	// Only perform the use if the shader id is different.
	if (CurrentShaderID != shader_id) {
		Shader* NextShader = GetByID(shader_id);
		CurrentShaderID = shader_id;
		if (!Renderer->UseRenderShader(NextShader)) {
			UL_ERROR("Failed to use shader '%s'.", NextShader->Name);
			return false;
		}
		if (!Renderer->BindGlobalsRenderShader(NextShader)) {
			UL_ERROR("Failed to bind globals for shader '%s'.", NextShader->Name);
			return false;
		}
	}
	return true;
}

unsigned short ShaderSystem::GetUniformIndex(Shader* shader, const char* uniform_name) {
	if (!shader || shader->ID == INVALID_ID) {
		UL_ERROR("shader_system_uniform_location called with invalid shader.");
		return INVALID_ID_U16;
	}

	unsigned short Index = INVALID_ID_U16;
	if (!Lookup.Get(uniform_name, &Index) || Index == INVALID_ID_U16) {
		UL_ERROR("Shader '%s' does not have a registered uniform named '%s'", shader->Name, uniform_name);
		return INVALID_ID_U16;
	}
	return shader->Uniforms[Index].index;
}

bool ShaderSystem::SetUniform(const char* uniform_name, const void* value) {
	if (CurrentShaderID == INVALID_ID) {
		UL_ERROR("shader_system_uniform_set called without a shader in use.");
		return false;
	}
	Shader* s = &Shaders[CurrentShaderID];
	unsigned short Index = GetUniformIndex(s, uniform_name);
	return SetUniformByIndex(Index, value);
}

bool ShaderSystem::SetSampler(const char* sampler_name, const Texture* tex) {
	return SetUniform(sampler_name, tex);
}

bool ShaderSystem::SetUniformByIndex(unsigned short index, const void* value) {
	Shader* Shader = &Shaders[CurrentShaderID];
	ShaderUniform* uniform = &Shader->Uniforms[index];
	if (Shader->BoundScope != uniform->scope) {
		if (uniform->scope == eShader_Scope_Global) {
			Renderer->BindGlobalsRenderShader(Shader);
		}
		else if (uniform->scope == eShader_Scope_Instance) {
			Renderer->BindInstanceRenderShader(Shader, Shader->BoundInstanceId);
		}
		else {
			// NOTE: Nothing to do here for locals, just set the uniform.
		}
		Shader->BoundScope = uniform->scope;
	}
	return Renderer->SetUniform(Shader, uniform, value);
}

bool ShaderSystem::SetSamplerByIndex(unsigned short index, const Texture* tex) {
	return SetUniformByIndex(index, tex);
}

bool ShaderSystem::ApplyGlobal() {
	return Renderer->ApplyGlobalRenderShader(&Shaders[CurrentShaderID]);
}

bool ShaderSystem::ApplyInstance() {
	return Renderer->ApplyInstanceRenderShader(&Shaders[CurrentShaderID]);
}

bool ShaderSystem::BindInstance(uint32_t instance_id) {
	Shader* s = &Shaders[CurrentShaderID];
	s->BoundInstanceId = instance_id;
	return Renderer->BindInstanceRenderShader(s, instance_id);
}

bool ShaderSystem::AddAttribute(Shader* shader, const ShaderAttributeConfig& config) {
	uint32_t Size = 0;
	switch (config.type) {
	case eShader_Attribute_Type_Int8:
	case eShader_Attribute_Type_UInt8:
		Size = 1;
		break;
	case eShader_Attribute_Type_Int16:
	case eShader_Attribute_Type_UInt16:
		Size = 2;
		break;
	case eShader_Attribute_Type_Float:
	case eShader_Attribute_Type_Int32:
	case eShader_Attribute_Type_UInt32:
		Size = 4;
		break;
	case eShader_Attribute_Type_Float_2:
		Size = 8;
		break;
	case eShader_Attribute_Type_Float_3:
		Size = 12;
		break;
	case eShader_Attribute_Type_Float_4:
		Size = 16;
		break;
	default:
		UL_ERROR("Unrecognized type %d, defaulting to size of 4. This probably is not what is desired.");
		Size = 4;
		break;
	}

	shader->AttributeStride += Size;

	// Create/push the attribute.
	ShaderAttribute Attrib = {};
	Attrib.name = (char*)Memory::Allocate(sizeof(char) * strlen(config.name), MemoryType::eMemory_Type_String);
	strcpy(Attrib.name, config.name);
	Attrib.size = Size;
	Attrib.type = config.type;

	shader->Attributes.push_back(Attrib);

	return true;
}

bool ShaderSystem::AddSampler(Shader* shader, ShaderUniformConfig& config) {
	if (config.scope == eShader_Scope_Instance && !shader->UseInstances) {
		UL_ERROR("add_sampler cannot add an instance sampler for a shader that does not use instances.");
		return false;
	}

	// Samples can't be used for push constants.
	if (config.scope == eShader_Scope_Local) {
		UL_ERROR("add_sampler cannot add a sampler at local scope.");
		return false;
	}

	// Verify the name is valid and unique.
	if (!IsUniformNameValid(shader, config.name) || !IsUniformAddStateValid(shader)) {
		return false;
	}

	// If global, push into the global list.
	uint32_t Location = 0;
	if (config.scope == eShader_Scope_Global) {
		uint32_t GlobalTextureCount = (uint32_t)shader->Attributes.size();
		if (GlobalTextureCount + 1 > Config.max_global_textures) {
			UL_ERROR("Shader global texture count %i exceeds max of %i", GlobalTextureCount, Config.max_global_textures);
			return false;
		}
		Location = GlobalTextureCount;
		shader->GlobalTextures.push_back(TextureSystem::GetDefaultTexture());
	}
	else {
		// Otherwise, it's instance-level, so keep count of how many need to be added during the resource acquisition.
		if (shader->InstanceTextureCount + 1 > Config.max_instance_textures) {
			UL_ERROR("Shader instance texture count %i exceeds max of %i", shader->InstanceTextureCount, Config.max_instance_textures);
			return false;
		}
		Location = shader->InstanceTextureCount;
		shader->InstanceTextureCount++;
	}

	// Treat it like a uniform. NOTE: In the case of samplers, out_location is used to determine the
	// hashtable entry's 'location' field value directly, and is then set to the index of the uniform array.
	// This allows location lookups for samplers as if they were uniforms as well (since technically they are).
	// TODO: might need to store this elsewhere
	if (!AddUniform(shader, config.name, 0, config.type, config.scope, Location, true)) {
		UL_ERROR("Unable to add sampler uniform.");
		return false;
	}

	return true;
}

bool ShaderSystem::AddUniform(Shader* shader, ShaderUniformConfig& config) {
	if (!IsUniformNameValid(shader, config.name) || !IsUniformAddStateValid(shader)) {
		return false;
	}

	return AddUniform(shader, config.name, config.size, config.type, config.scope, 0, false);
}

uint32_t ShaderSystem::GetShaderID(const char* shader_name) {
	uint32_t ShaderID = INVALID_ID;
	if (!Lookup.Get(shader_name, &ShaderID)) {
		UL_ERROR("There is no shader registered named '%s'.", shader_name);
		return INVALID_ID;
	}

	return ShaderID;
}

uint32_t ShaderSystem::NewShaderID() {
	for (uint32_t i = 0; i < Config.max_shader_count; ++i) {
		if (Shaders[i].ID == INVALID_ID) {
			return i;
		}
	}

	return INVALID_ID;
}

bool ShaderSystem::AddUniform(Shader* shader, const char* uniform_name, uint32_t size,
	ShaderUniformType type, ShaderScope scope, uint32_t set_location, bool is_sampler) {
	unsigned short UniformCount = (unsigned short)shader->Uniforms.size();
	if (UniformCount + 1 > Config.max_uniform_count) {
		UL_ERROR("A shader can only accept a combined maximum of %d uniforms and samplers at global, instance and local scopes.", Config.max_uniform_count);
		return false; 
	}
	
	ShaderUniform Entry;
	Entry.index = UniformCount;
	Entry.scope = scope;
	Entry.type = type;
	bool IsGlobal = (scope == eShader_Scope_Global);
	if (is_sampler) {
		Entry.location = set_location;
	}
	else {
		Entry.location = Entry.index;
	}

	if (scope != eShader_Scope_Local) {
		Entry.set_index = (uint32_t)scope;
		Entry.offset = is_sampler ? 0 : IsGlobal ? shader->GlobalUboSize : shader->UboSize;
		Entry.size = is_sampler ? 0 : size;
	}
	else {
		if (Entry.scope == eShader_Scope_Local && !shader->UseLocals) {
			UL_ERROR("Cannot add a locally-scoped uniform for a shader that does not support locals.");
			return false;
		}

		// Push a new aligned range (align to 4, as required by Vulkan spec)
		Entry.set_index = INVALID_ID_U8;
		Range r = PaddingAligned(shader->PushConstantsSize, size, 4);
		// utilize the aligned offset/range
		Entry.offset = r.offset;
		Entry.size = (unsigned short)r.size;

		// Track in configuration for use in initialization.
		shader->PushConstantsRanges[shader->PushConstantsRangeCount] = r;
		shader->PushConstantsRangeCount++;

		// Increase the push constant's size by the total value.
		shader->PushConstantsSize += r.size;
	}

	if (!shader->UniformLookup.Set(uniform_name, &Entry.index)) {
		UL_ERROR("Failed to add uniform.");
		return false;
	}

	shader->Uniforms.push_back(Entry);

	if (!is_sampler) {
		if (Entry.scope == eShader_Scope_Global) {
			shader->GlobalUboSize += Entry.size;
		}
		else if (Entry.scope == eShader_Scope_Instance) {
			shader->UboSize += Entry.size;
		}
	}

	return true;
}

bool ShaderSystem::IsUniformNameValid(Shader* shader, const char* uniform_name) {
	if (!uniform_name || strlen(uniform_name) == 0) {
		UL_ERROR("Uniform name must exist.");
		return false;
	}

	unsigned short location;
	if ( shader->UniformLookup.Get(uniform_name, &location) && location != INVALID_ID_U16) {
		UL_ERROR("A uniform by the name '%s' already exists on shader '%s'.", uniform_name, shader->Name);
		return false;
	}
	return true;
}

bool ShaderSystem::IsUniformAddStateValid(Shader* shader) {
	if (shader->State != eShader_State_Uninitialized) {
		UL_ERROR("Uniforms may only be added to shaders before initialization.");
		return false;
	}
	return true;
}