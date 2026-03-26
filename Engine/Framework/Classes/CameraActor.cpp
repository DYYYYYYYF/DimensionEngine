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
