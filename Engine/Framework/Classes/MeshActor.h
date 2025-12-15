#pragma once
#include "Actor.h"

class MeshActor : public Actor {
public:
	ENGINE_API MeshActor();
	ENGINE_API MeshActor(const std::string& name);
	ENGINE_API virtual ~MeshActor();

public:
	ENGINE_API virtual void Draw() = 0;
};