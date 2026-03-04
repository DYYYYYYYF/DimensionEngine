#pragma once

#include "MeshActor.h"
#include "Resources/Resource.hpp"
#include "Resources/Geometry.hpp"
#include "Framework/Components/TransformComponent.hpp"
#include <vector>

class ATextActor : public AMeshActor {
public:
	ATextActor() : AMeshActor(), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8) {}
	ATextActor(const FString& Name) : AMeshActor(Name), geometries(nullptr), geometry_count(0), Generation(INVALID_ID_U8) {}
	virtual ~ATextActor() { Unload(); }

public:
	DAPI virtual void Draw() override;

	DAPI void Unload();

public:
	unsigned char Generation;
	unsigned short geometry_count;
	Geometry** geometries;
};
