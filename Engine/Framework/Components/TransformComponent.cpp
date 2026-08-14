#include "TransformComponent.h"
#include "Framework/Classes/Actor.h"

void UTransformComponent::OnEnable() {
	// 开始时先更新一次，确保 LocalTransform 的矩阵是最新的
	UpdateTransform();
}

void UTransformComponent::Tick(float DeltaTime) {
	if (bTransformDirty) {
		UpdateTransform();
	}
}

const FTransform& UTransformComponent::GetLocalTransform() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform;
}

const Matrix4& UTransformComponent::GetLocalMatrix() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform.GetLocal();
}

const Matrix4& UTransformComponent::GetWorldMatrix() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return WorldTransform;
}

Vector3 UTransformComponent::TransformPointToWorld(const Vector3& LocalPoint) const {
	return LocalTransform.TransformPoint(LocalPoint);
}

void UTransformComponent::SetLocalTransform(const FTransform& Transform) {
	LocalTransform = Transform;
}

void UTransformComponent::SetLocation(const Vector3& Position) {
	if (Position.Equals(GetLocation())) return;
	LocalTransform.SetLocation(Position);
	MarkTransformDirty();
}

const Vector3& UTransformComponent::GetLocation() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform.GetLocation();
}

void UTransformComponent::SetQuaternion(const Quaternion& Rotation) {
	if (Rotation.Equals(GetLocation())) return;
	LocalTransform.SetQuaternion(Rotation);
	MarkTransformDirty();
}

const Quaternion& UTransformComponent::GetQuaternion() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform.GetQuaternion();
}

void UTransformComponent::SetRotation(const Vector3& Rotation) {
	if (Rotation.Equals(GetRotation())) return;
	LocalTransform.SetRotation(Rotation);
	MarkTransformDirty();
}

Vector3 UTransformComponent::GetRotation() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform.GetQuaternion().ToEuler();
}

void UTransformComponent::SetScale(const Vector3& Scale) {
	LocalTransform.SetScale(Scale);
	MarkTransformDirty();
}

const Vector3& UTransformComponent::GetScale() const {
	if (bTransformDirty) {
		UpdateTransform();
	}

	return LocalTransform.GetScale();
}

void UTransformComponent::Rotate(const Quaternion& Rotation) {
	LocalTransform.Rotate(Rotation);
	MarkTransformDirty();
}

void UTransformComponent::UpdateTransform() const {
	if (!bTransformDirty)
		return;

	LocalTransform.UpdateLocal();

	const Matrix4& LocalMat = LocalTransform.GetLocal();
	// 更新WorldTransform
	AActor* Owner = GetOwner();
	if (Owner) {
		AActor* OwnerParent = Owner->GetParent();
		if (OwnerParent) {
			const Matrix4& ParentMat = OwnerParent->GetWorldTransform();
			WorldTransform = ParentMat.Multiply(LocalMat);
		}
		else {
			WorldTransform = LocalMat;
		}
	}

	bTransformDirty = false;

	const_cast<UTransformComponent*>(this)->OnTransformChanged();
}

void UTransformComponent::MarkTransformDirty() {
	// 如果已经标记为脏，则无需再次标记
	if (bTransformDirty) return;
	bTransformDirty = true;
}
