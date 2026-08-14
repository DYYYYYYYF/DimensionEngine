#include "RotationCubeActor.h"
#include <Framework/Components/StaticMeshComponent.h>

ARotationCubeActor::ARotationCubeActor() : ACubeActor() {
	ARotationCubeActor("RotationCubeActor");
}

ARotationCubeActor::ARotationCubeActor(const FString& Name) : ACubeActor(Name) {
	SetEnableTick(true);
}

void ARotationCubeActor::Tick(float DeltaTime) {
	Quaternion RotationY = Quaternion(Axis::Y, 0.5f * (float)DeltaTime, false);
	Quaternion RotationX = Quaternion(Axis::X, 0.5f * (float)DeltaTime, false);
	MeshComponent->Rotate(RotationY);
	MeshComponent->Rotate(RotationX);

	// 执行父类方法
	ACubeActor::Tick(DeltaTime);
}
