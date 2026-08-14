#pragma once

#include "Component.h"
#include "Math/Transform.h"

/**
 * @brief Transform 组件。
 *
 * 多重继承 UBaseComponent（组件身份。
 * 不重复任何 FTransform 方法——所有位置、旋转、缩放、矩阵操作
 * 
 */
class ENGINE_API UTransformComponent : public UComponent
{
	DECLARE_CLASS_TYPE(UTransformComponent)

public:
	UTransformComponent() : UComponent("TransformComponent"), WorldTransform(Matrix4::Identity()) {}
	UTransformComponent(const FString& Name) : UComponent(Name), WorldTransform(Matrix4::Identity()) {}
	virtual void OnEnable() override;
	virtual void Tick(float DeltaTime) override;

public:

	void SetLocalTransform(const FTransform& Transform);
	const FTransform& GetLocalTransform() const;

	const Matrix4& GetLocalMatrix() const;
	const Matrix4& GetWorldMatrix() const;
	Vector3 TransformPointToWorld(const Vector3& LocalPoint) const;

	void SetLocation(const Vector3& Position);
	const Vector3& GetLocation() const;
	void SetQuaternion(const Quaternion& Rotation);
	const Quaternion& GetQuaternion() const;
	void SetRotation(const Vector3& Rotation);
	Vector3 GetRotation() const;
	void SetScale(const Vector3& Scale);
	const Vector3& GetScale() const;

	void Rotate(const Quaternion& Rotation);

	void UpdateTransform() const;

protected:
	virtual void OnTransformChanged() {}

private:
	void MarkTransformDirty();

private:
	mutable FTransform LocalTransform;
	mutable Matrix4 WorldTransform;
	mutable bool bTransformDirty = true;
};
