#include "Actor.h"

AActor::AActor() :ABaseObject(), ParentActor(nullptr) {
	LocalTransform = CreateComponent<UTransformComponent>();
	ASSERT(LocalTransform);
}

AActor::AActor(const FString& Name) : Name_(Name), ParentActor(nullptr) { 
	LocalTransform = CreateComponent<UTransformComponent>();
	ASSERT(LocalTransform);
}

void AActor::BeginPlay() {
	for (auto& Pair : ContainComponents) {
		if (Pair.Value && Pair.Value->IsEnabled()) {
			Pair.Value->OnEnable();
		}
	}
}

void AActor::Tick(float DeltaTime) {
	if (LocalTransform->IsDirty()) {
		LocalTransform->UpdateLocal();
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
	return LocalTransform->GetLocal();
}

Matrix4 AActor::GetWorldTransform() const {
	Matrix4 LocalMat = GetLocalTransform();
	if (ParentActor != nullptr) {
		Matrix4 ParentMat = ParentActor->GetWorldTransform();
		return ParentMat.Multiply(LocalMat);
	}

	return LocalMat;
}
