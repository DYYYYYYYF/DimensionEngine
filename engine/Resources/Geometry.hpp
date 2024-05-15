#pragma once

#include "Math/MathTypes.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 256

class Material;

class Geometry {
public:
	Geometry() : Material(nullptr) {}
	virtual ~Geometry(){}

public:
	uint32_t ID;
	uint32_t InternalID;
	uint32_t Generation;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	Material* Material;

};