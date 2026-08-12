#include "CameraActor.h"
#include "Framework/Components/CameraComponent.h"

// ============================================================
//  构造
// ============================================================

ACameraActor::ACameraActor()
	: AActor("CameraActor") {
	CameraComponent = CreateComponent<UCameraComponent>("CameraComponent");
}

ACameraActor::ACameraActor(const FString& Name)
	: AActor(Name) {
	CameraComponent = CreateComponent<UCameraComponent>("CameraComponent");
}

// ============================================================
//  生命周期
// ============================================================

void ACameraActor::BeginPlay() {
	AActor::BeginPlay();
	
}

void ACameraActor::Tick(float DeltaTime) {
	AActor::Tick(DeltaTime);
}

void ACameraActor::Destroy() {
	AActor::Destroy();
}

// 基础设置
void ACameraActor::SetFOV(float FOV) { GetCameraComponent()->SetFOV(FOV); }
float ACameraActor::GetFOV() const { return GetCameraComponent()->GetFOV(); }
void ACameraActor::SetAspectRatio(float AspectRatio) { GetCameraComponent()->SetAspectRatio(AspectRatio); }
float ACameraActor::GetAspectRatio() const { return GetCameraComponent()->GetAspectRatio(); }
void ACameraActor::SetNearPlane(float NearPlane) { GetCameraComponent()->SetNearPlane(NearPlane); }
float ACameraActor::GetNearPlane() const { return GetCameraComponent()->GetNearPlane(); }
void ACameraActor::SetFarPlane(float FarPlane) { GetCameraComponent()->SetFarPlane(FarPlane); }
float ACameraActor::GetFarPlane() const { return GetCameraComponent()->GetFarPlane(); }

Matrix4 ACameraActor::GetProjectionMatrix(ECameraProjectionMode Mode) const {
	switch (Mode) {
	case ECameraProjectionMode::Perspective:
		return Matrix4::Perspective(GetFOV(), GetAspectRatio(), GetNearPlane(), GetFarPlane());
	case ECameraProjectionMode::Orthographic:
		return Matrix4::Orthographic(0, GetAspectRatio() * GetFarPlane(), GetFarPlane(), 0.0f, GetNearPlane(), GetFarPlane());
	}

	return Matrix4::Identity();
}

Matrix4 ACameraActor::GetViewMatrix() const {
	return GetCameraComponent()->GetViewMatrix();
}

const FFrustum& ACameraActor::GetFrustum() {
	if (IsFrustumDirty) {
		Vector3 CameraPos = GetCameraComponent()->GetPosition();
		Vector3 CameraForward = GetCameraComponent()->Forward();
		Vector3 CameraRight = GetCameraComponent()->Right();
		Vector3 CameraUp = GetCameraComponent()->Up();
		float AspectRatio = GetAspectRatio();
		float FOV = Deg2Rad(GetFOV());
		float NearPlane = GetNearPlane();
		float FarPlane = GetFarPlane();
		Frustum = FFrustum(CameraPos, CameraForward, CameraRight, CameraUp, AspectRatio, FOV, NearPlane, FarPlane);
		IsFrustumDirty = false;
	}

	return Frustum;
}
