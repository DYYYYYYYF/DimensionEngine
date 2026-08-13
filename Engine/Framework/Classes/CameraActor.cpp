#include "CameraActor.h"
#include "Framework/Components/CameraComponent.h"

ACameraActor::ACameraActor()
	: AActor("CameraActor") {
	CameraComponent = CreateComponent<UCameraComponent>("CameraComponent");
	SetRootComponent(CameraComponent);
}

ACameraActor::ACameraActor(const FString& Name)
	: AActor(Name) {
	CameraComponent = CreateComponent<UCameraComponent>("CameraComponent");
	SetRootComponent(CameraComponent);
}

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
float ACameraActor::GetFOV() const { return CameraComponent->GetFOV(); }
void ACameraActor::SetFOV(float FOV) { CameraComponent->SetFOV(FOV); }
float ACameraActor::GetAspectRatio() const { return CameraComponent->GetAspectRatio(); }
void ACameraActor::SetAspectRatio(float AspectRatio) { CameraComponent->SetAspectRatio(AspectRatio); }
float ACameraActor::GetNearPlane() const { return CameraComponent->GetNearPlane(); }
void ACameraActor::SetNearPlane(float NearPlane) { CameraComponent->SetNearPlane(NearPlane); }
float ACameraActor::GetFarPlane() const { return CameraComponent->GetFarPlane(); }
void ACameraActor::SetFarPlane(float FarPlane) { CameraComponent->SetFarPlane(FarPlane); }

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

const FFrustum& ACameraActor::GetFrustum() const {
	return CameraComponent->GetFrustum();
}
