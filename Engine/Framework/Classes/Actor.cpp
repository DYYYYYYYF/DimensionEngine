#include "Actor.h"

AActor::AActor(const FString& Name) : Name_(Name), ParentActor(nullptr) { 
	RootComponent = CreateComponent<USceneComponent>("SceneComponent");
	ASSERT(RootComponent);
}

void AActor::BeginPlay() {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value && Pair.Value->IsEnabled()) {
			Pair.Value->OnEnable();
		}
	}
}

void AActor::RegisterComponents() {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value) {
			Pair.Value->OnRegister();
		}
	}
}

void AActor::Tick(float DeltaTime) {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value && Pair.Value->IsEnabled()) {
			Pair.Value->Tick(DeltaTime);
		}
	}
}

void AActor::UnregisterComponents() {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value) {
			Pair.Value->OnUnregister();
		}
	}
}

void AActor::Destroy() {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value && Pair.Value->IsEnabled()) {
			Pair.Value->OnDisable();
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
