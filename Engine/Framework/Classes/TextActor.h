#pragma once

#include "MeshActor.h"
#include "Resources/Resource.hpp"
#include "Resources/Geometry.hpp"
#include "Framework/Components/TransformComponent.hpp"
#include <vector>

class TextActor : public MeshActor {
public:
	TextActor() : MeshActor(), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8) {}
	TextActor(const FString& Name) : MeshActor(Name), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8) {}
	virtual ~TextActor() { Unload(); }

public:
	DAPI virtual void Draw() override;

	DAPI void Unload();

public:
	unsigned char Generation;
	unsigned short geometry_count;
	Geometry** geometries;
};
