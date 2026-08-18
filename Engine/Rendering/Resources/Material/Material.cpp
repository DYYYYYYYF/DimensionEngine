#include "Material.hpp"
#include "Systems/MaterialSystem.h"

UMaterial::UMaterial(const FString& Name) : UAsset(Name) {
	ReferenceCount = 0;
	AutoRelease = false;
	Generation = INVALID_ID;
	InternalID = INVALID_ID;
	ShaderID = INVALID_ID;
}

UMaterial::~UMaterial() {
	if (ReferenceCount > 0 && AutoRelease) {
		GLOG(Log::eWarn, "Material '%s' is being destroyed while still in use. Reference count: %zu", Name.CStr(), ReferenceCount);
	}
}

void UMaterial::DecreaseReferenceCount(uint32_t Count/* = 1*/){
	if (ReferenceCount <= 0) return;

	ReferenceCount -= Count; 
	if (ReferenceCount == 0) {
		// Material is no longer in use, trigger cleanup.
		DestroyInstance();
	}
}

void UMaterial::DestroyInstance() {
	MaterialSystem::Get().DestroyMaterial(this);
}


// --------------------------------- MaterialInstance ---------------------------------
UMaterialInstance::UMaterialInstance(UMaterial* BaseMat) : BaseMaterial(BaseMat) {
	RenderFrameNumer = INVALID_ID_U64; 
	if (BaseMaterial) {
		BaseMaterial->IncreaseReferenceCount(); 
		// Instance 初始时继承 Material 默认参数。 
		// 这里先复制一份，保持和你当前系统的数据结构兼容。 
		// 后面如果需要优化，可以改成 Override 模式。
		UniformValues = BaseMaterial->GetUniformValues(); 
		TextureBindings = BaseMaterial->GetTextureBindings(); 
	}

	SetInternalID(BaseMaterial->GetInternalID());
}

UMaterialInstance::~UMaterialInstance() {
	if (BaseMaterial) {
		BaseMaterial->DecreaseReferenceCount();
		BaseMaterial = nullptr;
	}
}

bool UMaterialInstance::IsTextureBindingExist(const FString& UniformName) const {
	for (const TextureBinding& binding : TextureBindings) {
		if (binding.uniform && binding.uniform->name == UniformName) {
			return true;
		}
	}
	return false;
}

bool UMaterialInstance::SetTextureOnBinding(const FString& UniformName, FTextureMap Texture) {
	for (TextureBinding& binding : TextureBindings) {
		if (binding.uniform && binding.uniform->name == UniformName) {
			binding.texture.texture = Texture.texture;
			return true;
		}
	}
	return false;
}

void UMaterialInstance::Release()
{
	if (BaseMaterial) {
		BaseMaterial->DecreaseReferenceCount();
	}
}
