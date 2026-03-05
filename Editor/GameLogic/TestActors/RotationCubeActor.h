#pragma once

#include "BaseActors/CubeActor.h"

class ARotationCubeActor : public ACubeActor {
public:
	ARotationCubeActor();
	ARotationCubeActor(const FString& Name);

	virtual void Tick(float DeltaTime) override;
};