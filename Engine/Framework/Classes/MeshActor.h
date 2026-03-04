#pragma once
#include "Actor.h"

class ENGINE_API AMeshActor : public AActor {
public:
	AMeshActor();
	AMeshActor(const FString& Name);
	virtual ~AMeshActor();

public:
	virtual void Draw() = 0;
};