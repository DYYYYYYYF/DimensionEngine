#pragma once
#include "Actor.h"

class ENGINE_API MeshActor : public Actor {
public:
	MeshActor();
	MeshActor(const FString& Name);
	virtual ~MeshActor();

public:
	virtual void Draw() = 0;
};