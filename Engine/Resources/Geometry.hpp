#pragma once

#include "Math/MathTypes.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 256

class Material;

class Geometry {
public:
	uint32_t ID;
	uint32_t InternalID;
	uint32_t Generation;
	Vec3 Center;
	Extents3D Extents;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	Material* Material = nullptr;

};