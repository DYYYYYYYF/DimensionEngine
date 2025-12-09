#include "Camera.hpp"

Camera::Camera() {
	Reset();
}

Camera::Camera(uint32_t id) {
	Reset();
	ID = id;
}

void Camera::Reset() {
	ID = INVALID_ID_U16;
	ReferenceCount = 0;
	EulerRotation = Vector3(0);
	Position = Vector3(0);
	IsDirty = false;
	ViewMatrix = Matrix4::Identity();
}

Vector3 Camera::GetPosition() {
	return Position;
}

void Camera::SetPosition(Vector3 pos) {
	Position = pos;
	IsDirty = true;
}

Vector3 Camera::GetEulerAngles() {
	return Vector3(Rad2Deg(EulerRotation.x), Rad2Deg(EulerRotation.y), Rad2Deg(EulerRotation.z));
}

void Camera::SetEulerAngles(Vector3 eular) {
	EulerRotation.x = Deg2Rad(eular.x);
	EulerRotation.y = Deg2Rad(eular.y);
	EulerRotation.z = Deg2Rad(eular.z);
	IsDirty = true;
}

void Camera::SetViewMatrix(const Matrix4& mat) {
	Position = mat.GetTranslation();
	CameraQuaternion = MatrixToQuat(mat);
	EulerRotation = CameraQuaternion.ToEuler();

	ViewMatrix = mat; 
	IsDirty = true;
}

Matrix4 Camera::GetViewMatrix() {
	if (IsDirty) {
		Matrix4 Rotation = Matrix4::EulerXYZ(EulerRotation.x, EulerRotation.y, EulerRotation.z);
		Matrix4 Translation = Matrix4::FromTranslation(Position);

		ViewMatrix = Translation.Multiply(Rotation);
		ViewMatrix = ViewMatrix.Inverse();

		IsDirty = false;
	}

	return ViewMatrix;
}

void Camera::MoveForward(float amount) {
	Vector3 Direction = Forward();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveBackward(float amount) {
	Vector3 Direction = Backward();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveLeft(float amount) {
	Vector3 Direction = Left();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveRight(float amount) {
	Vector3 Direction = Right();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveUp(float amount) {
	Vector3 Direction = Vector3::Up();
	Direction = Direction * amount;
	Position = Position + Direction;
	IsDirty = true;
}

void Camera::MoveDown(float amount) {
	Vector3 Direction = Vector3::Down();
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
