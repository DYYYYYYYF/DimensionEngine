#pragma once

#define GEOMETRY_NAME_MAX_LENGTH 256

class Material;

class Geometry {
public:
	Geometry() : Material(nullptr) {}
	virtual ~Geometry(){}

public:
	uint32_t ID = INVALID_ID;
	uint32_t InternalID = INVALID_ID;
	uint32_t Generation = INVALID_ID;
	char name[GEOMETRY_NAME_MAX_LENGTH];
	Material* Material = nullptr;

};