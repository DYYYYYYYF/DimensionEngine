#include "Material.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Rendering/Renderer.hpp"

#include <cstring>

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
	// 独立槽释放：若实例持有自分配的独立 instance 槽（非父材质共享槽），
	// 必须显式归还给后端，否则每次几何重建都会泄漏槽位/UBO/描述符集。
	if (bHasIndependentSlot && InternalID != INVALID_ID && BaseMaterial) {
		UShader* Shader = ShaderSystem::Get().GetByID(BaseMaterial->GetShaderID());
		IRenderer* Renderer = IRenderer::GetRenderer();
		if (Shader && Renderer) {
			Renderer->ReleaseInstanceResource(Shader, InternalID);
		}
	}

	if (BaseMaterial) {
		BaseMaterial->DecreaseReferenceCount();
		BaseMaterial = nullptr;
	}
}

bool UMaterialInstance::AcquireIndependentInstanceSlot() {
	if (bHasIndependentSlot) {
		return InternalID != INVALID_ID;
	}

	if (!BaseMaterial) {
		return false;
	}

	UShader* Shader = ShaderSystem::Get().GetByID(BaseMaterial->GetShaderID());
	if (!Shader) {
		GLOG(Log::eError, "UMaterialInstance::AcquireIndependentInstanceSlot failed to find shader for material '%s'.", BaseMaterial->GetName().CStr());
		return false;
	}

	TArray<FTextureMap*> Maps;
	for (TextureBinding& TB : TextureBindings) {
		Maps.Push(&TB.texture);
	}

	IRenderer* Renderer = IRenderer::GetRenderer();
	uint32_t NewID = Renderer->AcquireInstanceResource(Shader, Maps);
	if (NewID == INVALID_ID) {
		GLOG(Log::eError, "UMaterialInstance::AcquireIndependentInstanceSlot failed to acquire instance resource for material '%s'.", BaseMaterial->GetName().CStr());
		return false;
	}

	InternalID = NewID;
	bHasIndependentSlot = true;
	return true;
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

bool UMaterialInstance::SetUniformValue(const FString& UniformName, const void* Value, uint32_t Size) {
	for (UniformValue& UV : UniformValues) {
		if (UV.uniform && UV.uniform->name == UniformName) {
			const uint32_t CopySize = Size < sizeof(UV.data) ? Size : (uint32_t)sizeof(UV.data);
			if (Value) {
				memcpy(UV.data, Value, CopySize);
			}
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
