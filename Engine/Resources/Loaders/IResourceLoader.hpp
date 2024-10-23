#pragma once

#include "Resources/Resource.hpp"

class IResourceLoader {
public:
	IResourceLoader() : Id(INVALID_ID), CustomType(nullptr), TypePath(nullptr) {}
	~IResourceLoader() {
		Id = INVALID_ID;
		CustomType = nullptr;
		TypePath = nullptr;
	}

public:
	virtual bool Load(const char* name, void* params, Resource* resource) = 0;
	virtual void Unload(Resource* resouce) = 0;

public:
	uint32_t Id;
	ResourceType Type;
	const char* CustomType;
	const char* TypePath;

};
