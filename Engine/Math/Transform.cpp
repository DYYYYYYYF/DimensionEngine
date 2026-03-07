#include "Transform.h"

FTransform::FTransform() {
	SetPRS(Vector3(0.0f), Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

FTransform::FTransform(const FTransform& trans) {
	SetPRS(trans.GetLocation(), trans.GetQuaternion(), trans.GetScale());
	Local = trans.Local;
}

FTransform::FTransform(const Vector3& position) {
	SetPRS(position, Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

FTransform::FTransform(const Quaternion& rotation) {
	SetPRS(Vector3(0.0f), rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

FTransform::FTransform(const Vector3& position, const Quaternion& rotation) {
	SetPRS(position, rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

FTransform::FTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
	SetPRS(position, rotation, scale);
	Local = Matrix4::Identity();
}

void FTransform::Translate(const Vector3& translation) {
	vPosition = vPosition + translation;
	bIsDirty = true;
}

void FTransform::Rotate(const Quaternion& rotation) {
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void FTransform::Scale(const Vector3& scale) {
	vScale = vScale * scale;
	bIsDirty = true;
}

void FTransform::SetPR(const Vector3& pos, const Quaternion& rotation) {
	vPosition = pos;
	vRotation = rotation;
	bIsDirty = true;
}

void FTransform::SetPRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale) {
	vPosition = pos;
	vRotation = rotation;
	vScale = scale;
	bIsDirty = true;
}

void FTransform::TransformRotate(const Vector3& translation, const Quaternion& rotation) {
	vPosition = vPosition + translation;
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void FTransform::UpdateLocal() const {
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

Matrix4 FTransform::GetLocal() const {
	return Local;
}

Matrix4 FTransform::GetWorldMatrix() const {
	if (bIsDirty) {
		UpdateLocal();
	}
	return Local;
}

Matrix4 FTransform::GetInverseWorldMatrix() const {
	if (bIsDirty) {
		UpdateLocal();
	}

	if (bInverseDirty) {
		InverseLocal = Local.Inverse();
		bInverseDirty = false;
	}

	return InverseLocal;
}

Vector3 FTransform::TransformPoint(const Vector3& point) const {
	if (bIsDirty) {
		UpdateLocal();
	}
	return Local * point;
}

Vector3 FTransform::TransformDirection(const Vector3& direction) const {
	Matrix4 RS = Matrix4::FromScale(vScale) * vRotation.ToRotationMatrix();
	return RS * direction;
}

Vector3 FTransform::InverseTransformPoint(const Vector3& point) const {
	return GetInverseWorldMatrix() * point;
}

