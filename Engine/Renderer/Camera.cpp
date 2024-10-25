#include "Camera.hpp"

Camera::Camera() {
	Reset();
}

void Camera::Reset() {
	EulerRotation = Vec3(0);
	Position = Vec3(0);
	IsDirty = false;
	ViewMatrix = Matrix4::Identity();
}

Vec3 Camera::GetPosition() {
	return Position;
}

void Camera::SetPosition(Vec3 pos) {
	Position = pos;
	IsDirty = true;
}

Vec3 Camera::GetEulerAngles() {
	return EulerRotation;
}

void Camera::SetEulerAngles(Vec3 eular) {
	EulerRotation.x = Deg2Rad(eular.x);
	EulerRotation.y = Deg2Rad(eular.y);
	EulerRotation.z = Deg2Rad(eular.z);
	IsDirty = true;
}

Matrix4 Camera::GetViewMatrix() {
	if (IsDirty) {
		Matrix4 Rotation = Matrix4::EulerXYZ(EulerRotation.x, EulerRotation.y, EulerRotation.z);
		Matrix4 Translation = Matrix4::FromTranslation(Position);

		ViewMatrix = Rotation.Multiply(Translation);
		ViewMatrix = ViewMatrix.Inverse();

		IsDirty = false;
	}

	return ViewMatrix;
}

void Camera::MoveForward(float amount) {
	Vec3 Direction = Forward();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveBackward(float amount) {
	Vec3 Direction = Backward();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveLeft(float amount) {
	Vec3 Direction = Left();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveRight(float amount) {
	Vec3 Direction = Right();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveUp(float amount) {
	Vec3 Direction = Vec3::Up();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveDown(float amount) {
	Vec3 Direction = Vec3::Down();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::RotateYaw(float amount) {
	EulerRotation.y += amount;
	IsDirty = true;
}

void Camera::RotatePitch(float amount) {
	EulerRotation.x += amount;

	// Clamp to avoid Gimball lock.
	static const float limit = 1.55334306f;	// 89 defrees, or equivalent to Deg2Rad(89.0f);
	EulerRotation.x = CLAMP(EulerRotation.x, -limit, limit);

	IsDirty = true;
}
