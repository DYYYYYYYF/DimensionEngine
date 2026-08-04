#include "RotationCubeActor.h"

ARotationCubeActor::ARotationCubeActor() : ACubeActor() {
	ARotationCubeActor("RotationCubeActor");
}

ARotationCubeActor::ARotationCubeActor(const FString& Name) : ACubeActor(Name) {
	SetEnableTick(true);
}

void ARotationCubeActor::Tick(float DeltaTime) {
	Quaternion RotationY = Quaternion(Axis::Y, 0.5f * (float)DeltaTime, false);
	Quaternion RotationX = Quaternion(Axis::X, 0.5f * (float)DeltaTime, false);
	Rotate(RotationY);
	Rotate(RotationX);
	Rotate(RotationY);

	// 执行父类方法
	ACubeActor::Tick(DeltaTime);
}
