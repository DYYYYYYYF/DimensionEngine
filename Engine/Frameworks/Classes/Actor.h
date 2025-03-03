#pragma once

#include "Object.h"
#include "Frameworks/Components/ActorComponent.h"

class Actor : public Object{
public:
	ENGINE_API Actor();
	ENGINE_API virtual ~Actor() {}

public:
	ENGINE_API virtual void Tick(float DeltaTime);

public:
	ENGINE_API void SetLocation(const Vector3& Loc) { LocalTransform.SetLocation(Loc); }
	ENGINE_API Vector3 GetLocation() const { return LocalTransform.GetLocation(); }

	ENGINE_API void SetQuaternion(const Quaternion& Quat) { LocalTransform.SetQuaternion(Quat); }
	ENGINE_API Quaternion GetQuaternion() const { return LocalTransform.GetQuaternion(); }

	ENGINE_API void SetScale(const Vector3& Sca) { LocalTransform.SetScale(Sca); }
	ENGINE_API Vector3 GetScale() const { return LocalTransform.GetScale(); }

	ENGINE_API void Rotate(const Quaternion& Quat) { LocalTransform.Rotate(Quat); }

	ENGINE_API void SetTransform(const Transform& Trans) { LocalTransform = Trans; }
	ENGINE_API void SetTransform(const Vector3& Location, const Quaternion& Rotation) { 
		LocalTransform.SetLocation(Location); 
		LocalTransform.SetQuaternion(Rotation); 
	}
	ENGINE_API void SetTransform(const Vector3& Location, const Quaternion& Rotation, const Vector3& Scale) {
		LocalTransform.SetLocation(Location);
		LocalTransform.SetQuaternion(Rotation);
		LocalTransform.SetScale(Scale);
	}
	ENGINE_API Transform GetTransform() const { return LocalTransform; }

	ENGINE_API Matrix4 GetLocalTransform() { return LocalTransform.GetLocal(); }
	ENGINE_API Matrix4 GetWorldTransform();

	ENGINE_API bool AttachTo(Actor* Own);

protected:
	// 父对象
	Actor* Parent;
	// 对象组件
	ActorComponent* ActorComp;
	// Actor Transform
	Transform LocalTransform;

};