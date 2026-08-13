#include "Actor.h"

AActor::AActor(const FString& Name) : Name_(Name), ParentActor(nullptr) { 
	RootComponent = CreateComponent<USceneComponent>("SceneComponent");
	ASSERT(RootComponent);
}

bool AActor::Initialize() {
	for (auto Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component) {
				continue;
			}

			Component->Initialize();
		}
	}

	return true;
}

void AActor::BeginPlay() {
	// 启用
	for (auto& Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component || !Component->IsEnabled()) {
				continue;
			}

			Component->OnEnable();
		}
	}
}

void AActor::RegisterComponents() {
	for (auto& Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component || Component->IsRegistered()) {
				continue;
			}

			Component->OnRegister();
		}
	}
}

void AActor::Tick(float DeltaTime) {
	for (auto& Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component || !Component->IsEnabled()) {
				continue;
			}

			Component->Tick(DeltaTime);
		}
	}
}

void AActor::UnregisterComponents() {
	for (auto& Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component || !Component->IsRegistered()) {
				continue;
			}

			Component->OnUnregister();
		}
	}
}

void AActor::Destroy() {
	for (auto& Pair : ContainComponents) {
		for (UComponent* Component : Pair.Value) {
			if (!Component || !Component->IsEnabled()) {
				continue;
			}

			Component->OnDisable();
		}
	}

	ContainComponents.Clear(); 
}

bool AActor::AttachTo(AActor* Own) {
	if (Own) {
		ParentActor = Own;
		return true;
	}

	GLOG(Log::eWarn, "Invalid pointer.");
	return false;
}

bool AActor::AddChild(AActor* child) {
	if (!child) {
		return false;
	}

	child->ParentActor = this;
	ChildrenActors.Push(child);
	return true;
}

Matrix4 AActor::GetLocalTransform() const {
	return RootComponent->GetLocalMatrix();
}

Matrix4 AActor::GetWorldTransform() const {
	Matrix4 LocalMat = GetLocalTransform();
	if (ParentActor != nullptr) {
		Matrix4 ParentMat = ParentActor->GetWorldTransform();
		return ParentMat.Multiply(LocalMat);
	}

	return LocalMat;
}
