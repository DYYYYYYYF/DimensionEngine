#pragma once

#include "Rendering/Resources/Resource.hpp"
#include <string>

class IResourceLoader {
public:
	IResourceLoader() : Id(INVALID_ID), Type(ResourceType::eResource_type_Unkonw){}
	virtual ~IResourceLoader() {
		Id = INVALID_ID;
		Type = eResource_type_Unkonw;
	}

public:
	virtual bool Load(const std::string& name, void* params, Resource* resource) = 0;
	virtual void Unload(Resource* resouce) = 0;

public:
	uint32_t Id;
	ResourceType Type;
	std::string CustomType;
	std::string TypePath;

};
