#include "CameraComponent.h"
#include "Framework/Classes/Actor.h"

UCameraComponent::UCameraComponent(const FString& Name) :UComponent(Name) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	// 以 LocalTransform 的初始值初始化旋转状态
	Quaternion Q = LocalTransform->GetQuaternion();
	Matrix4 RotMat = Q.ToRotationMatrix();
	// 从旋转矩阵反解 Euler（通过 MatrixToQuat 再 ToEuler 保持一致性）
	EulerRotation_ = MatrixToQuat(RotMat).ToEuler();
	IsDirty_ = true;
}

void UCameraComponent::SetPosition(const Vector3& Pos) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(Pos);
	IsDirty_ = true;
}

Vector3 UCameraComponent::GetPosition() const {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return Vector3();
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return Vector3();
	}

	return LocalTransform->GetLocation(); 
}

void UCameraComponent::SetEulerAngles(const Vector3& EulerDeg) {
	EulerRotation_.x = Deg2Rad(EulerDeg.x);
	EulerRotation_.y = Deg2Rad(EulerDeg.y);
	EulerRotation_.z = Deg2Rad(EulerDeg.z);
	IsDirty_ = true;
	SyncToTransform();
}

Vector3 UCameraComponent::GetEulerAngles() const {
	return Vector3(
		Rad2Deg(EulerRotation_.x),
		Rad2Deg(EulerRotation_.y),
		Rad2Deg(EulerRotation_.z)
	);
}

void UCameraComponent::RebuildViewMatrix() {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	Matrix4 R = Matrix4::EulerXYZ(EulerRotation_.x, EulerRotation_.y, EulerRotation_.z);
	Matrix4 T = Matrix4::FromTranslation(LocalTransform->GetLocation());
	ViewMatrix_ = T.Multiply(R).Inverse();
	IsDirty_ = false;
}

Matrix4 UCameraComponent::GetViewMatrix() {
	if (IsDirty_) {
		RebuildViewMatrix();
	}
	return ViewMatrix_;
}

void UCameraComponent::SetViewMatrix(const Matrix4& Mat) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	// 从 ViewMatrix 反解 Position 和旋转，保持内部状态一致
	// ViewMatrix = Inverse(T * R)，所以 WorldMatrix = Inverse(ViewMatrix)
	Matrix4 WorldMat = Mat.Inverse();

	Vector3 Pos = WorldMat.GetTranslation();
	LocalTransform->SetLocation(Pos);

	Quaternion Q = MatrixToQuat(WorldMat);
	EulerRotation_ = Q.ToEuler();

	ViewMatrix_ = Mat;
	IsDirty_ = false;

	SyncToTransform();
}

void UCameraComponent::MoveForward(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Forward() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::MoveBackward(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Backward() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::MoveLeft(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Left() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::MoveRight(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Right() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::MoveUp(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Vector3::Up() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::MoveDown(float Amount) {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	LocalTransform->SetLocation(LocalTransform->GetLocation() + Vector3::Down() * Amount);
	IsDirty_ = true;
}

void UCameraComponent::RotateYaw(float Amount) {
	EulerRotation_.y += Amount;
	IsDirty_ = true;
	SyncToTransform();
}

void UCameraComponent::RotatePitch(float Amount) {
	EulerRotation_.x += Amount;
	EulerRotation_.x = CLAMP(EulerRotation_.x, -PitchLimit, PitchLimit);
	IsDirty_ = true;
	SyncToTransform();
}

void UCameraComponent::Reset() {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	EulerRotation_ = Vector3(0.0f);
	LocalTransform->SetLocation(Vector3(0.0f));
	LocalTransform->SetQuaternion(Quaternion());
	ViewMatrix_ = Matrix4::Identity();
	IsDirty_ = false;
}

void UCameraComponent::SyncToTransform() {
	AActor* Owner = GetOwner();
	if (!Owner) {
		return;
	}

	UTransformComponent* LocalTransform = Owner->GetTransformComponent();
	if (!LocalTransform) {
		return;
	}

	Matrix4    RotMat = Matrix4::EulerXYZ(EulerRotation_.x, EulerRotation_.y, EulerRotation_.z);
	Quaternion Quat = MatrixToQuat(RotMat);
	LocalTransform->SetQuaternion(Quat);
}