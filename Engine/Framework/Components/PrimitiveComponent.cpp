#include "PrimitiveComponent.h"
#include "Scene/World.h"
#include "Framework/Classes/Actor.h"
#include "Rendering/RenderWorld/RenderProxy.h"
#include "Rendering/RenderWorld/RenderWorld.h"

void UPrimitiveComponent::OnRegister()
{
	if (IsRegistered) {
		GLOG(Log::eWarn, "PrimitiveComponent is already registered in the RenderWorld.");
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	if (!CreateRenderProxy()) {
		GLOG(Log::eError, "Failed to create RenderProxy for PrimitiveComponent.");
		return;
	}
	
	// 注册到RenderWorld中
	URenderWorld* RenderWorld = World->GetRenderWorld();
	if (!RenderWorld) return;

	RenderWorld->AddProxy(RenderProxy);
	IsRegistered = true;

	// 注册到场景后更新
	UpdateRenderProxy();
}

void UPrimitiveComponent::OnUnregister() {
	if (RenderProxy) {
		// 检查是否已经注册
		if (!IsRegistered) {
			GLOG(Log::eWarn, "RenderProxy is not registered in the RenderWorld.");
		}

		// 从 RenderWorld 中移除 RenderProxy
		UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
		if (World) {
			World->GetRenderWorld()->RemoveProxy(RenderProxy);
		}
		else {
			GLOG(Log::eWarn, "Owner's world is null. Cannot remove RenderProxy from RenderWorld.");
		}

		DeleteObject(RenderProxy);
		RenderProxy = nullptr;
		IsRegistered = false;
	}
}

void UPrimitiveComponent::OnTransformChanged() {
	UpdateRenderProxy();
}
