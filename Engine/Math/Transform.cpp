#include "Transform.hpp"

Transform::Transform() {
	SetPRS(Vector3(0.0f), Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

Transform::Transform(const Transform& trans) {
	SetPRS(trans.GetLocation(), trans.GetQuaternion(), trans.GetScale());
	Local = trans.Local;
}

Transform::Transform(const Vector3& position) {
	SetPRS(position, Quaternion(), Vector3(1.0f));
	Local = Matrix4::Identity();
}

Transform::Transform(const Quaternion& rotation) {
	SetPRS(Vector3(0.0f), rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

Transform::Transform(const Vector3& position, const Quaternion& rotation) {
	SetPRS(position, rotation, Vector3(1.0f));
	Local = Matrix4::Identity();
}

Transform::Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
	SetPRS(position, rotation, scale);
	Local = Matrix4::Identity();
}

void Transform::Translate(const Vector3& translation) {
	vPosition = vPosition + translation;
	bIsDirty = true;
}

void Transform::Rotate(const Quaternion& rotation) {
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void Transform::Scale(const Vector3& scale) {
	vScale = vScale * scale;
	bIsDirty = true;
}

void Transform::SetPR(const Vector3& pos, const Quaternion& rotation) {
	vPosition = pos;
	vRotation = rotation;
	bIsDirty = true;
}

void Transform::SetPRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale) {
	vPosition = pos;
	vRotation = rotation;
	vScale = scale;
	bIsDirty = true;
}

void Transform::TransformRotate(const Vector3& translation, const Quaternion& rotation) {
	vPosition = vPosition + translation;
	vRotation = rotation.Multiply(vRotation);
	bIsDirty = true;
}

void Transform::UpdateLocal() {
	Matrix4 R = vRotation.ToRotationMatrix();
	Matrix4 T = Matrix4::FromTranslation(vPosition);
	Matrix4 S = Matrix4::FromScale(vScale);

	Local = T.Multiply(R.Multiply(S));
	bIsDirty = false;
}

Matrix4 Transform::GetLocal() {
	return Local;
}
