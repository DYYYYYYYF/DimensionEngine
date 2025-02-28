#include "Actor.h"

Actor::Actor():Object(), Parent(nullptr) {
	ActorComp = NewObject<ActorComponent>();
	if (ActorComp) {
		ActorComp->AttachTo(this);
	}
}

void Actor::Tick(float DeltaTime) {
	if (LocalTransform.IsDirty()) {
		LocalTransform.UpdateLocal();
	}

}

bool Actor::AttachTo(Actor* Own) {
	if (Own) {
		Parent = Own;
		return true;
	}

	LOG_WARN("Invalid pointer.");
	return false;
}

Matrix4 Actor::GetWorldTransform() {
	Matrix4 LocalMat = GetLocalTransform();
	if (Parent != nullptr) {
		Matrix4 ParentMat = Parent->GetWorldTransform();
		return ParentMat.Multiply(LocalMat);
	}

	return LocalMat;
}
