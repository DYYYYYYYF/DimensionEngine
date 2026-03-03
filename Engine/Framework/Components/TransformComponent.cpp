#include "TransformComponent.hpp"

TransformComponent::TransformComponent() {
	SetPRS(Vector3(0.0f), Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

TransformComponent::TransformComponent(const TransformComponent& trans) {
	SetPRS(trans.GetLocation(), trans.GetQuaternion(), trans.GetScale());
	Local = trans.Local;
}

TransformComponent::TransformComponent(const Vector3& position) {
	SetPRS(position, Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

TransformComponent::TransformComponent(const Quaternion& rotation) {
	SetPRS(Vector3(0.0f), rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

TransformComponent::TransformComponent(const Vector3& position, const Quaternion& rotation) {
	SetPRS(position, rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

TransformComponent::TransformComponent(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
	SetPRS(position, rotation, scale);
	Local = Matrix4::Identity();
}

void TransformComponent::Translate(const Vector3& translation) {
	vPosition = vPosition + translation;
	bIsDirty = true;
}

void TransformComponent::Rotate(const Quaternion& rotation) {
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void TransformComponent::Scale(const Vector3& scale) {
	vScale = vScale * scale;
	bIsDirty = true;
}

void TransformComponent::SetPR(const Vector3& pos, const Quaternion& rotation) {
	vPosition = pos;
	vRotation = rotation;
	bIsDirty = true;
}

void TransformComponent::SetPRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale) {
	vPosition = pos;
	vRotation = rotation;
	vScale = scale;
	bIsDirty = true;
}

void TransformComponent::TransformRotate(const Vector3& translation, const Quaternion& rotation) {
	vPosition = vPosition + translation;
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void TransformComponent::UpdateLocal() const {
	if (!bIsDirty) {
		return;
	}

	Matrix4 S = Matrix4::FromScale(vScale);
	Matrix4 R = vRotation.ToRotationMatrix();
	Matrix4 T = Matrix4::FromTranslation(vPosition);
	Local = T.Multiply(R.Multiply(S));

	bIsDirty = false;
	bInverseDirty = true; // 标记逆矩阵需要更新
}

Matrix4 TransformComponent::GetLocal() const {
	return Local;
}

Matrix4 TransformComponent::GetWorldMatrix() const {
	if (bIsDirty) {
		UpdateLocal();
	}
	return Local;
}

Matrix4 TransformComponent::GetInverseWorldMatrix() const {
	if (bIsDirty) {
		UpdateLocal();
	}

	if (bInverseDirty) {
		InverseLocal = Local.Inverse();
		bInverseDirty = false;
	}

	return InverseLocal;
}

Vector3 TransformComponent::TransformPoint(const Vector3& point) const {
	if (bIsDirty) {
		const_cast<TransformComponent*>(this)->UpdateLocal();
	}
	return Local * point;
}

Vector3 TransformComponent::TransformDirection(const Vector3& direction) const {
	Matrix4 RS = Matrix4::FromScale(vScale) * vRotation.ToRotationMatrix();
	return RS * direction;
}

Vector3 TransformComponent::InverseTransformPoint(const Vector3& point) const {
	return GetInverseWorldMatrix() * point;
}

