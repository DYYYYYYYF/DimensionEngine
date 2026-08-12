#include "SkyboxComponent.h"
#include "Framework/Classes/Actor.h"
#include "Systems/MaterialSystem.h"
#include "Systems/GeometrySystem.h"
#include "Rendering/RenderWorld/RenderProxy.h"

USkyboxComponent::USkyboxComponent(const FString& Name) : UStaticMeshComponent(Name) {
	FGeometryConfig SkyboxCubeConfig = GeometrySystem::Get().GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, Name, FString());

	// Clear out the material name.
	SkyboxGeometry = GeometrySystem::Get().AcquireFromConfig(SkyboxCubeConfig, true);
	if (!SkyboxGeometry) {
		return;
	}

	UMaterial* Material = MaterialSystem::Get().Acquire("Material.Builtin.Skybox");
	if (!Material) {
		return;
	}

	SkyboxGeometry->SetMaterial(Material);
}

bool USkyboxComponent::CreateRenderProxy() {
	RenderProxy = NewObject<FSkyboxRenderProxy>(MemoryType::eMemory_Type_Renderer);
	if (!RenderProxy) {
		GLOG(Log::eError, "Failed to create RenderProxy for StaticMeshComponent.");
		return false;
	}

	return true;
}

void USkyboxComponent::UpdateRenderProxy() {
	// 还未注册到场景
	if (!IsRegistered) return;

	FSkyboxRenderProxy* Proxy = Cast<FSkyboxRenderProxy*>(RenderProxy);
	if (!Proxy) {
		GLOG(Log::eError, "UTextComponent RenderProxy is null. Cannot update.");
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) {
		GLOG(Log::eError, "TextComponent has no owner.");
		return;
	}

	// 填充数据
	Proxy->SetMesh(SkyboxGeometry);
	Proxy->SetModelMatrix(Owner->GetWorldTransform());
	Proxy->SetUniqueID(Owner->GetUniqueID());
}

