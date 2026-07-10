#include "MaterialSystem.h"

#include "Core/EngineLogger.hpp"
#include "Math/MathTypes.hpp"
#include "Rendering/Renderer.hpp"

#include "Systems/TextureSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"

MaterialSystem& MaterialSystem::Get() {
	static MaterialSystem MaterialSystemInstance;
	return MaterialSystemInstance;
}

bool MaterialSystem::Initialize(IRenderer* renderer, SMaterialSystemConfig config) {
	if (config.max_material_count == 0) {
		GLOG(Log::eFatal, "Material system init failed. TextureSystemConfig.max_texture_count should > 0");
		return false;
	}

	if (renderer == nullptr) {
		GLOG(Log::eFatal, "Material system init failed. Renderer is nullptr.");
		return false;
	}

	if (Initilized) {
		return true;
	}

	MaterialSystemConfig = config;
	Renderer = renderer;

	// Invalidate all textures in the array.
	uint32_t Count = MaterialSystemConfig.max_material_count;
	RegisteredMaterials.resize(Count);

	// Create default textures for use in the system.
	if (!CreateDefaultMaterial()) {
		GLOG(Log::eFatal, "Create default material failed. Application quit now!");
		return false;
	}

	Initilized = true;
	return true;
}

void MaterialSystem::Shutdown() {
	// Destroy all loaded textures.
	for (Material* m : RegisteredMaterials) {
		if (m) {
			DestroyMaterial(m);
			DeleteObject(m);
			m = nullptr;
		}
	};
	RegisteredMaterials.clear();
	std::vector<Material*>().swap(RegisteredMaterials);

	if (DefaultMaterial) {
		DestroyMaterial(DefaultMaterial);
		DeleteObject(DefaultMaterial);
		DefaultMaterial = nullptr;
	}

	MaterialMap.clear();
}

Material* MaterialSystem::Acquire(const FString& name) {
	if (name.Compare(DEFAULT_MATERIAL_NAME) == 0) {
		return DefaultMaterial;
	}

	// Load the given material configuration from disk.
	UAsset MatResource;
	if (!ResourceSystem::Get().Load(name, EAssetType::Material, nullptr, &MatResource)) {
		GLOG(Log::eError, "Failed to load material resource, returning nullptr.");
		return nullptr;
	}

	// Now acquire from loaded config.
	Material* Mat = nullptr;
	if (MatResource.Data) {
		Mat = AcquireFromConfig(*(SMaterialConfig*)MatResource.Data);
	}

	// Clean up
	ResourceSystem::Get().Unload(&MatResource);

	if (Mat == nullptr) {
		GLOG(Log::eError, "Failed to load material resource, returning nullptr.");
		return nullptr;
	}

	return Mat;
}

Material* MaterialSystem::AcquireFromConfig(SMaterialConfig config) {
	// Return default material.
	if (config.name.Compare(DEFAULT_MATERIAL_NAME) == 0) {
		return DefaultMaterial;
	}

	// 如果找不到材质，则创建一个新的材质。
	if (MaterialMap.find(config.name) == MaterialMap.end()) {
		uint32_t Count = MaterialSystemConfig.max_material_count;
		Material* m = nullptr;
		for (uint32_t i = 0; i < Count; ++i) {
			if (RegisteredMaterials[i] == nullptr) {
				// A free slot has been found. Use it index as the handle.
				RegisteredMaterials[i] = NewObject<Material>();
				RegisteredMaterials[i]->SetID(i);
				MaterialMap[config.name] = i;
				m = RegisteredMaterials[i];
				break;
			}
		}

		// Make sure an empty slot was actually found.
		if (m == nullptr || m->GetID() == INVALID_ID) {
			GLOG(Log::eFatal, "Material acquire failed. Material system cannot hold anymore materials. Adjust configuration to allow more.");
			return nullptr;
		}

		// Create new material.
		if (!LoadMaterial(config, m)) {
			GLOG(Log::eError, "Load %s material failed.", config.name.CStr());
			return nullptr;
		}

		if (m->Generation == INVALID_ID) {
			m->Generation = 1;
		}
		else {
			m->Generation++;
		}
	}

	uint32_t MaterialID = MaterialMap[config.name];
	Material* Mat = GetDefaultMaterial();
	if (MaterialID != INVALID_ID) {
		Mat = RegisteredMaterials[MaterialID];
	}
	ASSERT(Mat != nullptr);

	// This can only be changed the first time a material is loaded.
	if (Mat->GetReferenceCount() == 0) {
		Mat->SetIsAutoRelease(config.auto_release);
	}

	Mat->IncreaseReferenceCount();
	GLOG(Log::eDebug, "Material '%s' Reference count increased to %i.", config.name.CStr(), Mat->GetReferenceCount());

	// Update the entry.
	return Mat;
}

void MaterialSystem::Release(const FString& name) {
	// Ignore release requests for the default material.
	if (name.Compare(DEFAULT_MATERIAL_NAME) == 0) {
		return;
	}

	// Take a copy of name, it will be zero-out in DestroyMaterial();
	FString CopyMatName = name;

	if (MaterialMap.find(CopyMatName) != MaterialMap.end()) {
		uint32_t MaterialID = MaterialMap[CopyMatName];
		Material* Mat = RegisteredMaterials[MaterialID];
		if (Mat->GetReferenceCount() == 0) {
			GLOG(Log::eWarn, "Tried to release non-existent material: %s", CopyMatName.CStr());
			return;
		}

		Mat->DecreaseReferenceCount();
		if (Mat->GetReferenceCount() == 0 && Mat->IsAutoRelease()) {
			// Release material.
			DestroyMaterial(Mat);
			DeleteObject(Mat);
			RegisteredMaterials[MaterialID] = nullptr;
			GLOG(Log::eInfo, "Released material '%s'. Material unloaded.", CopyMatName.CStr());
		}

		// Update the entry.
		MaterialMap.erase(CopyMatName);
	}
}

Material* MaterialSystem::GetDefaultMaterial() {
	if (Initilized) {
		return DefaultMaterial;
	}

	return nullptr;
}

bool MaterialSystem::LoadMaterial(SMaterialConfig config, Material* mat) {
	// name
	mat->Name = config.name;
	mat->ShaderID = ShaderSystem::Get().GetID(config.shader_name);
	Shader* s = ShaderSystem::Get().GetByID(mat->ShaderID);
	if (!s)
	{
		GLOG(Log::eError, "Unable to load material because its shader was not found: '%s'. This is likely a problem with the material asset.", config.shader_name.CStr());
		return false;
	}

	// 由Shader反射具体Property后在config中查找
	const std::vector<ShaderUniform>& Uniforms = s->GetUnifromList();
	for (const ShaderUniform& uniform: Uniforms) {
		switch (uniform.scope)
		{
		case eShader_Scope_Global:
		case eShader_Scope_Local:
			continue;

		default:
			break;
		}

		// 查找Config
		auto property = config.Properties.Find(uniform.name);

		// 解析字符数据
		switch (uniform.type)
		{
		case eShader_Uniform_Type_Float:
		{
			float value = property ? property->ToFloat() : 0.0f;

			UniformValue MatValue;
			MatValue.uniform = s->GetUniformHandle(uniform.name);

			// 清零目标区域（保证未使用的字节干净）
			std::memset(MatValue.data, 0, 64);
			std::memcpy(MatValue.data, &value, sizeof(value));
			mat->UnifromValues.Push(std::move(MatValue));
		}
		break;

		case eShader_Uniform_Type_UInt32:
		{
			Vector4 vec = property? Vector4::FromString((*property).CStr()) : Vector4();

			UniformValue MatValue;
			MatValue.uniform = s->GetUniformHandle(uniform.name);

			// 清零目标区域（保证未使用的字节干净）
			std::memset(MatValue.data, 0, 64);
			std::memcpy(MatValue.data, &vec, sizeof(vec));
			mat->UnifromValues.Push(std::move(MatValue));
		}
		break;

		case eShader_Uniform_Type_Sampler:
		{
			TextureBinding texValue;
			texValue.uniform = s->GetUniformHandle(uniform.name);

			// 默认使用引擎初始贴图
			TextureUsage usage = GetTextureUsageFromUniformName(uniform.name);
			switch (usage)
			{
			case TextureUsage::eTexture_Usage_Map_Normal:
			{ 
				texValue.texture.texture = TextureSystem::Get().GetDefaultNormalTexture();
			}
			break;

			case TextureUsage::eTexture_Usage_Map_RoughnessMetallic:
			{ 
				texValue.texture.texture = TextureSystem::Get().GetDefaultRoughnessMetallicTexture(); 
			}
			break;
			default:
			{ 
				texValue.texture.texture = TextureSystem::Get().GetDefaultDiffuseTexture(); 
			}
			break;
			}

			// 如果材质有指定贴图则使用
			if (property) {
				CreateTextureMap(texValue.texture, usage, *property);
			}

			mat->TextureBindings.Push(std::move(texValue));
		}
		break;
		}
	}

	// Gather a list of pointers to texture maps.
	std::vector<TextureMap*> Maps;
	for (TextureBinding& TexBinding : mat->TextureBindings) {
		Maps.push_back(&(TexBinding.texture));
	}

	mat->InternalID = Renderer->AcquireInstanceResource(s, Maps);
	if (mat->InternalID == INVALID_ID) {
		GLOG(Log::eError, "Failed to acquire renderer resources for material '%s'.", mat->Name.CStr());
		return false;
	}

	return true;
}

void MaterialSystem::DestroyMaterial(Material* mat) {
	GLOG(Log::eInfo, "Destroying material '%s'...", mat->Name.CStr());

	// Release texture references.
	TextureSystem& TextureSystemInst = TextureSystem::Get();
	for (TextureBinding& TexBinding : mat->TextureBindings) {
		if (!TexBinding.texture.texture) {
			TextureSystemInst.Release(TexBinding.texture.texture->GetName());
		}
		Renderer->ReleaseTextureMap(&TexBinding.texture);
	}

	//Release renderer resources.
	if (mat->ShaderID != INVALID_ID && mat->InternalID != INVALID_ID) {
		Shader* s = ShaderSystem::Get().GetByID(mat->ShaderID);
		Renderer->ReleaseInstanceResource(s, mat->InternalID);
		mat->ShaderID = INVALID_ID;
	}

	// Zero it out, invalidate Ids.
	mat->SetID(INVALID_ID);
	mat->Generation = INVALID_ID;
	mat->InternalID = INVALID_ID;
	mat->RenderFrameNumer = INVALID_ID;
}

bool MaterialSystem::CreateDefaultMaterial() {
	if (DefaultMaterial) {
		GLOG(Log::eWarn, "Already exist default material.");
		return true;
	}

	//TextureSystem& TextureSystemInst = TextureSystem::Get();

	//DefaultMaterial = NewObject<Material>();
	//DefaultMaterial->SetID(INVALID_ID);
	//DefaultMaterial->Generation = INVALID_ID;
	//DefaultMaterial->Name = DEFAULT_MATERIAL_NAME;
	//DefaultMaterial->DiffuseColor = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	//DefaultMaterial->DiffuseMap.usage = TextureUsage::eTexture_Usage_Map_Diffuse;
	//DefaultMaterial->DiffuseMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->DiffuseMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->DiffuseMap.repeat_u = eTexture_Repeat_Repeat;
	//DefaultMaterial->DiffuseMap.repeat_v = eTexture_Repeat_Repeat;
	//DefaultMaterial->DiffuseMap.repeat_w = eTexture_Repeat_Repeat;
	//DefaultMaterial->DiffuseMap.texture = TextureSystemInst.GetDefaultDiffuseTexture();
	//if (!Renderer->AcquireTextureMap(&DefaultMaterial->DiffuseMap)) {
	//	GLOG(Log::eError, "Unable to acquire resources for diffuse texture map.");
	//	return false;
	//}

	//DefaultMaterial->NormalMap.usage = TextureUsage::eTexture_Usage_Map_Normal;
	//DefaultMaterial->NormalMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->NormalMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->NormalMap.repeat_u = eTexture_Repeat_Repeat;
	//DefaultMaterial->NormalMap.repeat_v = eTexture_Repeat_Repeat;
	//DefaultMaterial->NormalMap.repeat_w = eTexture_Repeat_Repeat;
	//DefaultMaterial->NormalMap.texture = TextureSystemInst.GetDefaultNormalTexture();
	//if (!Renderer->AcquireTextureMap(&DefaultMaterial->NormalMap)) {
	//	GLOG(Log::eError, "Unable to acquire resources for diffuse texture map.");
	//	return false;
	//}

	//DefaultMaterial->RoughnessMetallicMap.usage = TextureUsage::eTexture_Usage_Map_Normal;
	//DefaultMaterial->RoughnessMetallicMap.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->RoughnessMetallicMap.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	//DefaultMaterial->RoughnessMetallicMap.repeat_u = eTexture_Repeat_Repeat;
	//DefaultMaterial->RoughnessMetallicMap.repeat_v = eTexture_Repeat_Repeat;
	//DefaultMaterial->RoughnessMetallicMap.repeat_w = eTexture_Repeat_Repeat;
	//DefaultMaterial->RoughnessMetallicMap.texture = TextureSystemInst.GetDefaultNormalTexture();
	//if (!Renderer->AcquireTextureMap(&DefaultMaterial->RoughnessMetallicMap)) {
	//	GLOG(Log::eError, "Unable to acquire resources for diffuse texture map.");
	//	return false;
	//}

	//std::vector<TextureMap*> Maps = { &DefaultMaterial->DiffuseMap, &DefaultMaterial->NormalMap, &DefaultMaterial->RoughnessMetallicMap };

	//Shader* s = ShaderSystem::Get().Get("Shader.Builtin.GBuffer");
	//if (s == nullptr) {
	//	GLOG(Log::eFatal, "Shader.Builtin.GBuffer shader is nullptr.");
	//	ASSERT(s);
	//	return false;
	//}

	//DefaultMaterial->InternalID = Renderer->AcquireInstanceResource(s, Maps);
	//if (DefaultMaterial->InternalID == INVALID_ID) {
	//	GLOG(Log::eError, "Create default material failed. Application quit now!");
	//	return false;
	//}

	//// Make sure to assign the shader id.
	//DefaultMaterial->ShaderID = s->ID;

	return true;
}

bool MaterialSystem::CreateTextureMap(TextureMap& map, TextureUsage usage, const FString& textureName) {
	map.filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	map.filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	map.repeat_u = TextureRepeat::eTexture_Repeat_Repeat;
	map.repeat_v = TextureRepeat::eTexture_Repeat_Repeat;
	map.repeat_w = TextureRepeat::eTexture_Repeat_Repeat;

	if (!Renderer->AcquireTextureMap(&map)) return false;

	map.usage = usage;

	TextureSystem& texSys = TextureSystem::Get();

	if (!textureName.IsEmpty())
	{
		map.texture = texSys.Acquire(textureName, true);
	}

	if (!map.texture)
	{
		switch (usage)
		{
		case TextureUsage::eTexture_Usage_Map_Diffuse:
			map.texture = texSys.GetDefaultDiffuseTexture();
			break;

		case TextureUsage::eTexture_Usage_Map_Normal:
			map.texture = texSys.GetDefaultNormalTexture();
			break;

		case TextureUsage::eTexture_Usage_Map_RoughnessMetallic:
			map.texture = texSys.GetDefaultRoughnessMetallicTexture();
			break;
		}
	}

	return true;
}

TextureUsage MaterialSystem::GetTextureUsageFromUniformName(const FString& name) const {
	if (name.Compare("diffuse_texture") == 0) {
		return TextureUsage::eTexture_Usage_Map_Diffuse;
	}
	else if (name.Compare("normal_texture") == 0) {
		return TextureUsage::eTexture_Usage_Map_Normal;
	}
	else if (name.Compare("roughnessMetillc_texture") == 0) {
		return TextureUsage::eTexture_Usage_Map_RoughnessMetallic;
	}
	else if (name.Compare("cubemap_texture") == 0) {
		return TextureUsage::eTexture_Usage_Map_Cubemap;
	}
	else if (name.Compare("specular_texture") == 0) {
		return TextureUsage::eTexture_Usage_Map_Specular;
	}

	return TextureUsage::eTexture_Usage_Unknown;
}

#ifdef LEVEL_DEBUG
#define MATERIAL_APPLY_OR_FAIL(expr)                  \
    if (!expr) {                                      \
        GLOG(Log::eError, "Failed to apply material: %s", #expr); \
        return false;                                 \
    }
#else
#define MATERIAL_APPLY_OR_FAIL(expr) expr
#endif

bool MaterialSystem::ApplyGlobal(uint32_t shader_id, size_t renderer_frame_number, 
	const Matrix4& projection, const Matrix4& view, const Vector4& ambient_color, 
	const Vector3& view_position, uint32_t render_mode, float global_time) {

	Shader* UsedShader = ShaderSystem::Get().GetByID(shader_id);
	if (UsedShader == nullptr) {
		return false;
	}

	if (UsedShader->RenderFrameNumber == renderer_frame_number) {
		return true;
	}

	std::vector<ShaderUniform> uniforms = UsedShader->GetUnifromList();
	for (ShaderUniform& uniform : uniforms) {
		if (uniform.scope != eShader_Scope_Global) continue;

		switch (uniform.semantic)
		{
		case ShaderSemantic::eShaderSemantic_Projection:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &projection));
			break;

		case ShaderSemantic::eShaderSemantic_View:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &view));
			break;

		case ShaderSemantic::eShaderSemantic_ViewPosition:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &view_position));
			break;

		case ShaderSemantic::eShaderSemantic_AmbientColor:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &ambient_color));
			break;

		case ShaderSemantic::eShaderSemantic_Time:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &global_time));
			break;

		case ShaderSemantic::eShaderSemantic_RenderMode:
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(&uniform, &render_mode));
			break;

		default:
			break;
		}
	}

	MATERIAL_APPLY_OR_FAIL(UsedShader->ApplyGlobal());

	// Sync
	UsedShader->RenderFrameNumber = renderer_frame_number;

	return true;
}

bool MaterialSystem::ApplyInstance(Material* mat, bool need_update) {
	if (mat->InternalID == INVALID_ID) {
		return false;
	}

	Shader* UsedShader = ShaderSystem::Get().GetByID(mat->ShaderID);

	// Apply instance-level uniforms.
	MATERIAL_APPLY_OR_FAIL(UsedShader->BindInstance(mat->InternalID));
	if (need_update) {
		for (const auto& value : mat->UnifromValues)
		{
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(value.uniform, value.data));
		}

		for (const auto& tex : mat->TextureBindings)
		{
			MATERIAL_APPLY_OR_FAIL(UsedShader->SetUniform(tex.uniform, &tex.texture));
		}
	}

	MATERIAL_APPLY_OR_FAIL(UsedShader->ApplyInstance(need_update));
	return true;
}

bool MaterialSystem::ApplyLocal(Material* mat, const Matrix4& model) {
	Shader* UsedShader = ShaderSystem::Get().GetByID(mat->ShaderID);
	std::vector<ShaderUniform> uniforms = UsedShader->GetUnifromList();

	for (ShaderUniform& uniform : uniforms)
	{
		if (uniform.scope != eShader_Scope_Local) continue;

		switch (uniform.semantic)
		{
		case ShaderSemantic::eShaderSemantic_Model_Matrix:
			return UsedShader->SetUniform(&uniform, &model);
		}
	}

	return false;
}
