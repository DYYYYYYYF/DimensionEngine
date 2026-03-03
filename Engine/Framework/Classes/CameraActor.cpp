#include "CameraActor.hpp"

// ============================================================
//  构造
// ============================================================

ACameraActor::ACameraActor()
	: Actor("CameraActor") {
	ViewMatrix_ = Matrix4::Identity();
}

ACameraActor::ACameraActor(const std::string& Name)
	: Actor(Name) {
	ViewMatrix_ = Matrix4::Identity();
}

// ============================================================
//  生命周期
// ============================================================

void ACameraActor::BeginPlay() {
	Actor::BeginPlay();
	// 以 LocalTransform 的初始值初始化旋转状态
	// （允许外部在 BeginPlay 前通过 Actor::SetLocation 等接口预设位置）
	Quaternion Q = LocalTransform.GetQuaternion();
	Matrix4 RotMat = Q.ToRotationMatrix();
	// 从旋转矩阵反解 Euler（通过 MatrixToQuat 再 ToEuler 保持一致性）
	EulerRotation_ = MatrixToQuat(RotMat).ToEuler();
	IsDirty_ = true;
}

void ACameraActor::Tick(float DeltaTime) {
	Actor::Tick(DeltaTime);
}

void ACameraActor::Destroy() {
	Actor::Destroy();
}

// ============================================================
//  位置
// ============================================================

void ACameraActor::SetPosition(const Vector3& Pos) {
	LocalTransform.SetLocation(Pos);
	IsDirty_ = true;
}

// ============================================================
//  旋转
// ============================================================

void ACameraActor::SetEulerAngles(const Vector3& EulerDeg) {
	EulerRotation_.x = Deg2Rad(EulerDeg.x);
	EulerRotation_.y = Deg2Rad(EulerDeg.y);
	EulerRotation_.z = Deg2Rad(EulerDeg.z);
	IsDirty_ = true;
	SyncToTransform();
}

Vector3 ACameraActor::GetEulerAngles() const {
	return Vector3(
		Rad2Deg(EulerRotation_.x),
		Rad2Deg(EulerRotation_.y),
		Rad2Deg(EulerRotation_.z)
	);
}

// ============================================================
//  ViewMatrix
// ============================================================

void ACameraActor::RebuildViewMatrix() {
	Matrix4 R = Matrix4::EulerXYZ(EulerRotation_.x, EulerRotation_.y, EulerRotation_.z);
	Matrix4 T = Matrix4::FromTranslation(LocalTransform.GetLocation());
	ViewMatrix_ = T.Multiply(R).Inverse();
	IsDirty_ = false;
}

Matrix4 ACameraActor::GetViewMatrix() {
	if (IsDirty_) {
		RebuildViewMatrix();
	}
	return ViewMatrix_;
}

void ACameraActor::SetViewMatrix(const Matrix4& Mat) {
	// 从 ViewMatrix 反解 Position 和旋转，保持内部状态一致
	// ViewMatrix = Inverse(T * R)，所以 WorldMatrix = Inverse(ViewMatrix)
	Matrix4 WorldMat = Mat.Inverse();

	Vector3 Pos = WorldMat.GetTranslation();
	LocalTransform.SetLocation(Pos);

	Quaternion Q = MatrixToQuat(WorldMat);
	EulerRotation_ = Q.ToEuler();

	ViewMatrix_ = Mat;
	IsDirty_ = false;

	SyncToTransform();
}

// ============================================================
//  移动
// ============================================================

void ACameraActor::MoveForward(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Forward() * Amount);
	IsDirty_ = true;
}

void ACameraActor::MoveBackward(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Backward() * Amount);
	IsDirty_ = true;
}

void ACameraActor::MoveLeft(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Left() * Amount);
	IsDirty_ = true;
}

void ACameraActor::MoveRight(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Right() * Amount);
	IsDirty_ = true;
}

void ACameraActor::MoveUp(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Vector3::Up() * Amount);
	IsDirty_ = true;
}

void ACameraActor::MoveDown(float Amount) {
	LocalTransform.SetLocation(LocalTransform.GetLocation() + Vector3::Down() * Amount);
	IsDirty_ = true;
}

// ============================================================
//  旋转
// ============================================================

void ACameraActor::RotateYaw(float Amount) {
	EulerRotation_.y += Amount;
	IsDirty_ = true;
	SyncToTransform();
}

void ACameraActor::RotatePitch(float Amount) {
	EulerRotation_.x += Amount;
	EulerRotation_.x = CLAMP(EulerRotation_.x, -PitchLimit, PitchLimit);
	IsDirty_ = true;
	SyncToTransform();
}

// ============================================================
//  重置
// ============================================================

void ACameraActor::Reset() {
	EulerRotation_ = Vector3(0.0f);
	LocalTransform.SetLocation(Vector3(0.0f));
	LocalTransform.SetQuaternion(Quaternion());
	ViewMatrix_ = Matrix4::Identity();
	IsDirty_ = false;
}

// ============================================================
//  同步 EulerRotation_ → LocalTransform.Quaternion
//  移动操作只改位置，不需要调此函数；
//  旋转操作（RotateYaw/RotatePitch/SetEulerAngles）改变 EulerRotation_ 后调用。
// ============================================================
void ACameraActor::SyncToTransform() {
	Matrix4    RotMat = Matrix4::EulerXYZ(EulerRotation_.x, EulerRotation_.y, EulerRotation_.z);
	Quaternion Quat = MatrixToQuat(RotMat);
	LocalTransform.SetQuaternion(Quat);
}