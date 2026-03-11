#pragma once

#include "GeometryType.hpp"
#include "Rendering/Resources/Asset.hpp"

class Material;

class DAPI Geometry : public UAsset {
public:
	Geometry();
	Geometry(const FString& name);

public:
	uint32_t ID;
	uint32_t InternalID;
	uint32_t Generation;
	Vector3 Center;
	Extents3D Extents;
	FString name;
	Material* Material = nullptr;

};