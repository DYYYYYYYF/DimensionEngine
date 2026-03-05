#include "Actor.h"

AActor::AActor() :ABaseObject(), Parent(nullptr) {}
AActor::AActor(const FString& Name) : ABaseObject(), Name_(Name), Parent(nullptr) {}

void AActor::Tick(float DeltaTime) {
	if (LocalTransform.IsDirty()) {
		LocalTransform.UpdateLocal();
	}

}

bool AActor::AttachTo(AActor* Own) {
	if (Own) {
		Parent = Own;
		return true;
	}

	GLOG(Log::eWarn, "Invalid pointer.");
	return false;
}

Matrix4 AActor::GetWorldTransform() {
	Matrix4 LocalMat = GetLocalTransform();
	if (Parent != nullptr) {
		Matrix4 ParentMat = Parent->GetWorldTransform();
		return ParentMat.Multiply(LocalMat);
	}

	return LocalMat;
}
