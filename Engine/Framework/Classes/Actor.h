#pragma once

#include "BaseObject.h"
#include "Containers/FString.hpp"
#include "Framework/Components/TransformComponent.h"

class ENGINE_API AActor : public ABaseObject {
public:
	AActor();
	AActor(const FString& Name);
	virtual ~AActor() {}

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

	void SetTransform(const UTransformComponent& Trans) { LocalTransform = Trans; }
	void SetTransform(const Vector3& Location, const Quaternion& Rotation) {
		LocalTransform.SetLocation(Location); 
		LocalTransform.SetQuaternion(Rotation); 
	}
	void SetTransform(const Vector3& Location, const Quaternion& Rotation, const Vector3& Scale) {
		LocalTransform.SetLocation(Location);
		LocalTransform.SetQuaternion(Rotation);
		LocalTransform.SetScale(Scale);
	}
	UTransformComponent GetTransform() const { return LocalTransform; }

	Matrix4 GetLocalTransform() { return LocalTransform.GetLocal(); }
	Matrix4 GetWorldTransform();

	bool AttachTo(AActor* Own);

	void SetName(const FString& Name) { Name_ = Name; }
	FString GetName() const { return Name_; }

protected:
	FString Name_;
	// 父对象
	AActor* Parent;
	// Actor Transform
	UTransformComponent LocalTransform;

};