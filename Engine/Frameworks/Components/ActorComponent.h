#pragma once

#include "Math/MathTypes.hpp"
#include "Math/Transform.hpp"
#include "Frameworks/Classes/Object.h"

class Actor;

class ActorComponent : public Object {
public:
	ENGINE_API ActorComponent() : Object(), Owner(nullptr) {}
	ENGINE_API ~ActorComponent() {}

public:
	ENGINE_API void SetLocation(const Vector3& Loc) { RelativeTransform.SetLocation(Loc); }
	ENGINE_API Vector3 GetLocation() const { return RelativeTransform.GetLocation(); }

	ENGINE_API void SetQuaternion(const Quaternion& Quat) { RelativeTransform.SetQuaternion(Quat); }
	ENGINE_API Quaternion GetQuaternion() const { return RelativeTransform.GetQuaternion(); }

	ENGINE_API void SetScale(Vector3 Sca) { RelativeTransform.SetScale(Sca); }
	ENGINE_API Vector3 GetScale() const { return RelativeTransform.GetScale(); }

	ENGINE_API Transform GetTransform() const { return RelativeTransform; }
	ENGINE_API Matrix4 GetLocalTransform() { return RelativeTransform.GetLocal(); }

	ENGINE_API Actor* GetOwner() const { return Owner; }
	ENGINE_API bool AttachTo(Actor* Own);

protected:
	Actor* Owner;
	Transform RelativeTransform;


};
