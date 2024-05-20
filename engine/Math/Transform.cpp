#include "Transform.hpp"

Transform::Transform() {
	SetPRS(Vec3(0.0f), Quaternion::Identity(), Vec3(1.0f));
	Local = Matrix4::Identity();
	Parent = nullptr;
}

Transform::Transform(Vec3 position) {
	SetPRS(position, Quaternion::Identity(), Vec3(1.0f));
	Local = Matrix4::Identity();
	Parent = nullptr;
}

Transform::Transform(Quaternion rotation) {
	SetPRS(Vec3(0.0f), rotation, Vec3(1.0f));
	Local = Matrix4::Identity();
	Parent = nullptr;
}

Transform::Transform(Vec3 position, Quaternion rotation) {
	SetPRS(position, rotation, Vec3(1.0f));
	Local = Matrix4::Identity();
	Parent = nullptr;
}

Transform::Transform(Vec3 position, Quaternion rotation, Vec3 scale) {
	SetPRS(position, rotation, scale);
	Local = Matrix4::Identity();
	Parent = nullptr;
}

void Transform::Translate(Vec3 translation) {
	vPosition = vPosition + translation;
	IsDirty = true;
}

void Transform::Rotate(Quaternion rotation) {
	vRotation.QuaternionMultiply(rotation);
	IsDirty = true;
}

void Transform::Scale(Vec3 scale) {
	vScale = vScale * scale;
	IsDirty = true;
}

void Transform::SetPR(Vec3 pos, Quaternion rotation) {
	vPosition = pos;
	vRotation = rotation;
	IsDirty = true;
}

void Transform::SetPRS(Vec3 pos, Quaternion rotation, Vec3 scale) {
	vPosition = pos;
	vRotation = rotation;
	vScale = scale;
	IsDirty = true;
}

void Transform::TransformRotate(Vec3 translation, Quaternion rotation) {
	vPosition = vPosition + translation;
	vRotation.QuaternionMultiply(rotation);
	IsDirty = true;
}

Matrix4 Transform::GetLocal() {
	if (IsDirty) {
		Matrix4 Tr = QuatToMatrix(vRotation).Multiply(Matrix4::FromTranslation(vPosition));
		Tr = Matrix4::FromScale(vScale).Multiply(Tr);
		Local = Tr;
		IsDirty = false;
	}

	return Local;
}

Matrix4 Transform::GetWorldTransform() {
	Matrix4 l = GetLocal();
	if (Parent != nullptr) {
		Matrix4 p = Parent->GetWorldTransform();
		return l.Multiply(p);
	}

	return l;
}