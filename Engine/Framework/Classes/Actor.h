#pragma once

#include "BaseObject.h"
#include "Framework/Components/TransformComponent.hpp"

class ENGINE_API Actor : public BaseObject {
public:
	Actor();
	Actor(std::string Name);
	virtual ~Actor() {}

public:
	virtual void BeginPlay() {};
	virtual void Tick(float DeltaTime);
	virtual void Destroy() {};

public:
	void SetLocation(const Vector3& Loc) { LocalTransform.SetLocation(Loc); }
	Vector3 GetLocation() const { return LocalTransform.GetLocation(); }

	void SetQuaternion(const Quaternion& Quat) { LocalTransform.SetQuaternion(Quat); }
	Quaternion GetQuaternion() const { return LocalTransform.GetQuaternion(); }

	void SetScale(const Vector3& Sca) { LocalTransform.SetScale(Sca); }
	Vector3 GetScale() const { return LocalTransform.GetScale(); }

	void Rotate(const Quaternion& Quat) { LocalTransform.Rotate(Quat); }

	void SetTransform(const TransformComponent& Trans) { LocalTransform = Trans; }
	void SetTransform(const Vector3& Location, const Quaternion& Rotation) {
		LocalTransform.SetLocation(Location); 
		LocalTransform.SetQuaternion(Rotation); 
	}
	void SetTransform(const Vector3& Location, const Quaternion& Rotation, const Vector3& Scale) {
		LocalTransform.SetLocation(Location);
		LocalTransform.SetQuaternion(Rotation);
		LocalTransform.SetScale(Scale);
	}
	TransformComponent GetTransform() const { return LocalTransform; }

	Matrix4 GetLocalTransform() { return LocalTransform.GetLocal(); }
	Matrix4 GetWorldTransform();

	bool AttachTo(Actor* Own);

	std::string GetName() const { return Name_; }

protected:
	std::string Name_;
	// 父对象
	Actor* Parent;
	// Actor Transform
	TransformComponent LocalTransform;

};