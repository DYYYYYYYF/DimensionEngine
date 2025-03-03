#include "ShaderSystem.h"

#include "Renderer/RendererFrontend.hpp"
#include "Systems/TextureSystem.h"

#include "Core/Event.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Containers/TString.hpp"
#include "Renderer/Vulkan/VulkanShader.hpp"
#include "../Utils/JSONReader.h"

IRenderer* ShaderSystem::Renderer = nullptr;
ShaderSystem::Config ShaderSystem::ShaderSystemConfig;
std::unordered_map<std::string, uint32_t> ShaderSystem::ShaderMap;

uint32_t ShaderSystem::CurrentShaderID;
std::vector<Shader*> ShaderSystem::Shaders;
bool ShaderSystem::Initilized = false;
ShaderLanguage ShaderSystem::GLOBAL_SHADER_TYPE = ShaderLanguage::eGLSL;

bool OnReloadShader(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	std::string ShaderName = context.data.c;
	if (!ShaderSystem::ReloadShader(ShaderName)) {
		LOG_ERROR("Failed to reload shader %s.", ShaderName.c_str());
		return false;
	}

	return true;
}

bool ShaderSystem::Initialize(IRenderer* renderer, ShaderSystem::Config config) {
	if (renderer == nullptr) {
		return false;
	}

	// Read current config
	JSONReader JsonReader(ENGINE_CONFIG_PATH);
	GLOBAL_SHADER_TYPE = JsonReader.ReadPropertyString("ShaderLanguage")
		.compare("glsl") == 0 ? ShaderLanguage::eGLSL : ShaderLanguage::eHLSL;

	Renderer = renderer;
	if (config.max_shader_count < 512) {
		if (config.max_shader_count == 0) {
			LOG_ERROR("shader_system_initialize - config.max_shader_count must be greater than 0");
			return false;
		}
		else {
			// This is to help avoid hashtable collisions.
			LOG_ERROR("shader_system_initialize - config.max_shader_count is recommended to be at least 512.");
		}
	}
	
	// Figure out how large of a hashtable is needed.
	// Block of memory will contain state structure then the block for the hashtable.
	Shaders.resize(config.max_shader_count);
	ShaderSystemConfig = config;
	CurrentShaderID = INVALID_ID;

	if (!EngineEvent::Register(eEventCode::Reload_Shader_Module, nullptr, OnReloadShader)) {
		LOG_ERROR("Unable to listen for refresh required event, creation failed.");
		return false;
	}

	Initilized = true;
	return true;
}

void ShaderSystem::Shutdown() {
	if (Initilized) {
		for (uint32_t i = 0; i < ShaderSystemConfig.max_shader_count; ++i) {
			Shader* s = Shaders[i];
			if (s != nullptr) {
				if (s->ID != INVALID_ID) {
					s->Destroy();
				}

				DeleteObject(s);
				s = nullptr;
			}
		}

		EngineEvent::Unregister(eEventCode::Reload_Shader_Module, nullptr, OnReloadShader);

		ShaderMap.clear();
		std::vector<Shader*>().swap(Shaders);

		// Save current config
		JSONReader JsonReader(std::string(ROOT_PATH) + "/Engine/EngineConfig.json");
		std::string Lan = GLOBAL_SHADER_TYPE == ShaderLanguage::eGLSL ? "glsl" : "hlsl";
		JsonReader.SetPropertyString("ShaderLanguage", Lan);
	}
}

bool ShaderSystem::ReloadShader(const std::string& shader_name, ShaderLanguage language) {
	Shader* s = Get(shader_name);
	if (s == nullptr) {
		return false;
	}

	return ReloadShader(s, language);
}

bool ShaderSystem::ReloadShader(Shader* shader, ShaderLanguage language) {
	// Change shader status.
	shader->Status = ShaderStatus::eShader_State_Reloading;
	GLOBAL_SHADER_TYPE = language;

	if (!shader->Reload()) {
		LOG_ERROR("shader_system_create: reload shader failed for shader '%s'.", shader->Name.c_str());
		// NOTE: initialize automatically destroys the shader if it fails.
		shader->Destroy();
		return false;
	}


	shader->Status = ShaderStatus::eShader_State_Initialized;
	return true;
}

bool ShaderSystem::Create(IRenderpass* pass, ShaderConfig* config) {
	uint32_t ID = GetShaderID(config->name);
	if (ID == INVALID_ID) {
		ID = NewShaderID();
		ShaderMap[config->name] = ID;
	}
	else {
		LOG_WARN("Shader named '%s' already create. It will be covered.", config->name);
	}

	RendererBackendType BackendAPI = Renderer->GetBackendType();
	switch (BackendAPI)
	{
	case eRenderer_Backend_Type_Vulkan:
		Shaders[ID] = NewObject<VulkanShader>(Renderer);
		break;
		// TODO
	case eRenderer_Backend_Type_OpenGL:
		break;
	case eRenderer_Backend_Type_DirecX:
		break;
	default:
		Shaders[ID] = NewObject<VulkanShader>(Renderer);
		break;
	}
	
	Shader* OutShader = Shaders[ID];
	OutShader->ID = ID;
	if (OutShader->ID == INVALID_ID) {
		LOG_ERROR("Unable to find free slot to create new shader. Aborting.");
		return false;
	}
	
	OutShader->Name = StringCopy(config->name);
	OutShader->Language = GLOBAL_SHADER_TYPE;
	Memory::Zero(OutShader->PushConstantsRanges, sizeof(Range) * 32);

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

	// Process flags.
	OutShader->Flags = ShaderFlags::eShader_Flag_None;
	if (config->depthTest) {
		OutShader->Flags |= ShaderFlags::eShader_Flag_DepthTest;
	}
	if (config->depthWrite) {
		OutShader->Flags |= ShaderFlags::eShader_Flag_DepthWrite;
	}


	if (!Renderer->CreateRenderShader(OutShader, config, pass, config->stage_filenames, config->stages)) {
		LOG_ERROR("Error creating shader.");
		return false;
	}

	// Process attributes.
	for (uint32_t i = 0; i < config->attributes.size(); ++i) {
		AddAttribute(OutShader, config->attributes[i]);
	}

	// Process uniforms.
	for (uint32_t i = 0; i < config->uniforms.size(); ++i) {
		if (config->uniforms[i].type == eShader_Uniform_Type_Sampler) {
			AddSampler(OutShader, config->uniforms[i]);
		}
		else {
			AddUniform(OutShader, config->uniforms[i]);
		}
	}

	// Initialize the shader.
	if (!OutShader->Initialize()) {
		LOG_ERROR("shader_system_create: initialization failed for shader '%s'.", config->name);
		// NOTE: initialize automatically destroys the shader if it fails.
		return false;
	}

	OutShader->Status = ShaderStatus::eShader_State_Initialized;
	return true;
}

unsigned ShaderSystem::GetID(const char* shader_name) {
	return GetShaderID(shader_name);
}

Shader* ShaderSystem::GetByID(uint32_t shader_id) {
	if (shader_id >= ShaderSystemConfig.max_shader_count || Shaders[shader_id]->ID == INVALID_ID) {
		return nullptr;
	}

	return Shaders[shader_id];
}

Shader* ShaderSystem::Get(const std::string& shader_name) {
	uint32_t ShaderID = GetShaderID(shader_name);
	if (ShaderID != INVALID_ID) {
		return GetByID(ShaderID);
	}

	return nullptr;
}

void ShaderSystem::DestroyShader(Shader* s) {
	s->Destroy();
}

void ShaderSystem::Destroy(const char* shader_name) {
	uint32_t ShaderID = GetShaderID(shader_name);
	if (ShaderID == INVALID_ID) {
		return;
	}

	Shader* s = Shaders[ShaderID];
	s->Destroy();
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
			LOG_ERROR("Failed to use shader '%s'.", NextShader->Name);
			return false;
		}
		if (!Renderer->BindGlobalsRenderShader(NextShader)) {
			LOG_ERROR("Failed to bind globals for shader '%s'.", NextShader->Name);
			return false;
		}
	}
	return true;
}

unsigned short ShaderSystem::GetUniformIndex(Shader* shader, const char* uniform_name) {
	if (!shader || shader->ID == INVALID_ID) {
		LOG_ERROR("shader_system_uniform_location called with invalid shader.");
		return INVALID_ID_U16;
	}

	auto it = shader->HashMap.find(uniform_name);
	if (it == shader->HashMap.end()){
		LOG_ERROR("Shader '%s' does not have a registered uniform named '%s'", shader->Name, uniform_name);
		return INVALID_ID_U16;
	}

	unsigned short Index = it->second;
	if ( Index == INVALID_ID_U16) {
		LOG_ERROR("Shader '%s' does not have a registered uniform named '%s'", shader->Name, uniform_name);
		return INVALID_ID_U16;
	}

	return shader->Uniforms[Index].index;
}

bool ShaderSystem::SetUniform(const char* uniform_name, const void* value) {
	if (uniform_name == nullptr || value == nullptr) {
		LOG_ERROR("ShaderSystem::SetUniform called without a invalid uniform_name or value.");
		return false;
	}

	if (CurrentShaderID == INVALID_ID) {
		LOG_ERROR("ShaderSystem::SetUniform called without a shader in use.");
		return false;
	}
	Shader* s = Shaders[CurrentShaderID];
	unsigned short Index = GetUniformIndex(s, uniform_name);
	return SetUniformByIndex(Index, value);
}

bool ShaderSystem::SetSampler(const char* sampler_name, const Texture* tex) {
	return SetUniform(sampler_name, tex);
}

bool ShaderSystem::SetUniformByIndex(unsigned short index, const void* value) {
	if (index == INVALID_ID_U16 || value == nullptr) {
		LOG_WARN("ShaderSystem::SetUniformByIndex failed! It looks like out of boundings or invalid value. index: %d value: %#x", index, value);
		return false;
	}

	Shader* Shader = Shaders[CurrentShaderID];
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
	return Renderer->ApplyGlobalRenderShader(Shaders[CurrentShaderID]);
}

bool ShaderSystem::ApplyInstance(bool need_update) {
	return Renderer->ApplyInstanceRenderShader(Shaders[CurrentShaderID], need_update);
}

bool ShaderSystem::BindGlobal(uint32_t instance_id) {
	Shader* s = Shaders[CurrentShaderID];
	s->BoundInstanceId = instance_id;
	return Renderer->BindGlobalsRenderShader(s);
}

bool ShaderSystem::BindInstance(uint32_t instance_id) {
	Shader* s = Shaders[CurrentShaderID];
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
		LOG_ERROR("Unrecognized type %d, defaulting to size of 4. This probably is not what is desired.");
		Size = 4;
		break;
	}

	shader->AttributeStride += Size;

	// Create/push the attribute.
	ShaderAttribute Attrib = {};
	Attrib.name = StringCopy(config.name);
	Attrib.size = Size;
	Attrib.type = config.type;

	shader->Attributes.push_back(Attrib);

	return true;
}

bool ShaderSystem::AddSampler(Shader* shader, ShaderUniformConfig& config) {
	// Samples can't be used for push constants.
	if (config.scope == eShader_Scope_Local) {
		LOG_ERROR("add_sampler cannot add a sampler at local scope.");
		return false;
	}

	// Verify the name is valid and unique.
	if (!IsUniformNameValid(shader, config.name) || !IsUniformAddStateValid(shader)) {
		return false;
	}

	// If global, push into the global list.
	uint32_t Location = 0;
	if (config.scope == eShader_Scope_Global) {
		uint32_t GlobalTextureCount = (uint32_t)shader->GlobalTextureMaps.size();
		if (GlobalTextureCount + 1 > ShaderSystemConfig.max_global_textures) {
			LOG_ERROR("Shader global texture count %i exceeds max of %i", GlobalTextureCount, ShaderSystemConfig.max_global_textures);
			return false;
		}
		Location = GlobalTextureCount;

		// NOTE: Create a default texture map to be used here. Can always be updated later.
		TextureMap DefaultMap;
		DefaultMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
		DefaultMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
		DefaultMap.repeat_u = TextureRepeat::eTexture_Repeat_Repeat;
		DefaultMap.repeat_v = TextureRepeat::eTexture_Repeat_Repeat;
		DefaultMap.repeat_w = TextureRepeat::eTexture_Repeat_Repeat;
		DefaultMap.usage = TextureUsage::eTexture_Usage_Unknown;
		if (!Renderer->AcquireTextureMap(&DefaultMap)) {
			LOG_ERROR("Failed to acquire for global texture map during shader creation.");
			return false;
		}

		// Allocate a pointer assign the texture, and push into global texture maps.
		// NOTE: This allocation is only done for global texture maps.
		TextureMap* Map = (TextureMap*)Memory::Allocate(sizeof(TextureMap), MemoryType::eMemory_Type_Renderer);
		*Map = DefaultMap;
		Map->texture = TextureSystem::GetDefaultDiffuseTexture();
		shader->GlobalTextureMaps.push_back(Map);
	}
	else {
		// Otherwise, it's instance-level, so keep count of how many need to be added during the resource acquisition.
		if (shader->InstanceTextureCount + 1 > ShaderSystemConfig.max_instance_textures) {
			LOG_ERROR("Shader instance texture count %i exceeds max of %i", shader->InstanceTextureCount, ShaderSystemConfig.max_instance_textures);
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
		LOG_ERROR("Unable to add sampler uniform.");
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

uint32_t ShaderSystem::GetShaderID(const std::string& shader_name) {
	uint32_t ShaderID = INVALID_ID;
	auto it = ShaderMap.find(shader_name);
	if (it == ShaderMap.end()){
		return INVALID_ID;
	}

	return it->second;
}

uint32_t ShaderSystem::NewShaderID() {
	for (uint32_t i = 0; i < ShaderSystemConfig.max_shader_count; ++i) {
		if (Shaders[i] == nullptr || Shaders[i]->ID == INVALID_ID) {
			return i;
		}
	}

	return INVALID_ID;
}

bool ShaderSystem::AddUniform(Shader* shader, const char* uniform_name, uint32_t size,
	ShaderUniformType type, ShaderScope scope, uint32_t set_location, bool is_sampler) {
	unsigned short UniformCount = (unsigned short)shader->Uniforms.size();
	if (UniformCount + 1 > ShaderSystemConfig.max_uniform_count) {
		LOG_ERROR("A shader can only accept a combined maximum of %d uniforms and samplers at global, instance and local scopes.", ShaderSystemConfig.max_uniform_count);
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
		// Push a new aligned range (align to 4, as required by Vulkan spec)
		Entry.set_index = INVALID_ID_U16;
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

	shader->HashMap[uniform_name] = Entry.index;
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
		LOG_ERROR("Uniform name must exist.");
		return false;
	}

	auto it = shader->HashMap.find(uniform_name);
	if (it != shader->HashMap.end()){
		LOG_ERROR("A uniform by the name '%s' already exists on shader '%s'.", uniform_name, shader->Name);
		return false;
	}

	return true;
}

bool ShaderSystem::IsUniformAddStateValid(Shader* shader) {
	if (shader->Status != ShaderStatus::eShader_State_Uninitialized) {
		LOG_ERROR("Uniforms may only be added to shaders before initialization.");
		return false;
	}
	return true;
}
