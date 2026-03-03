#pragma once

#include "BaseObject.h"
#include "Framework/Components/TransformComponent.hpp"

class Actor : public BaseObject {
public:
	ENGINE_API Actor();
	ENGINE_API Actor(std::string Name);
	ENGINE_API virtual ~Actor() {}

public:
	ENGINE_API virtual void BeginPlay() {};
	ENGINE_API virtual void Tick(float DeltaTime);
	ENGINE_API virtual void Destroy() {};

public:
	ENGINE_API void SetLocation(const Vector3& Loc) { LocalTransform.SetLocation(Loc); }
	ENGINE_API Vector3 GetLocation() const { return LocalTransform.GetLocation(); }

	ENGINE_API void SetQuaternion(const Quaternion& Quat) { LocalTransform.SetQuaternion(Quat); }
	ENGINE_API Quaternion GetQuaternion() const { return LocalTransform.GetQuaternion(); }

	ENGINE_API void SetScale(const Vector3& Sca) { LocalTransform.SetScale(Sca); }
	ENGINE_API Vector3 GetScale() const { return LocalTransform.GetScale(); }

	ENGINE_API void Rotate(const Quaternion& Quat) { LocalTransform.Rotate(Quat); }

	ENGINE_API void SetTransform(const TransformComponent& Trans) { LocalTransform = Trans; }
	ENGINE_API void SetTransform(const Vector3& Location, const Quaternion& Rotation) { 
		LocalTransform.SetLocation(Location); 
		LocalTransform.SetQuaternion(Rotation); 
	}
	ENGINE_API void SetTransform(const Vector3& Location, const Quaternion& Rotation, const Vector3& Scale) {
		LocalTransform.SetLocation(Location);
		LocalTransform.SetQuaternion(Rotation);
		LocalTransform.SetScale(Scale);
	}
	ENGINE_API TransformComponent GetTransform() const { return LocalTransform; }

	ENGINE_API Matrix4 GetLocalTransform() { return LocalTransform.GetLocal(); }
	ENGINE_API Matrix4 GetWorldTransform();

	ENGINE_API bool AttachTo(Actor* Own);

	std::string GetName() const { return Name_; }

protected:
	std::string Name_;
	// 父对象
	Actor* Parent;
	// Actor Transform
	TransformComponent LocalTransform;

};