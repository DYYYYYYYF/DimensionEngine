#pragma once

#include "Actor.h"

class USkyboxComponent;

class ENGINE_API ASkyboxActor : public AActor {
	DECLARE_CLASS_TYPE(ASkyboxActor)

public:
	ASkyboxActor(const FString& Name);

protected:
	USkyboxComponent* SkyboxComponent;

};