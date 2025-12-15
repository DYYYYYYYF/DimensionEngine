#include "Actor.h"

Actor::Actor() :BaseObject(), Parent(nullptr) {}
Actor::Actor(std::string Name) : BaseObject(), Name_(Name), Parent(nullptr) {}

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

	GLOG(Log::eWarn, "Invalid pointer.");
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
