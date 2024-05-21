#pragma once

#include "Math/MathTypes.hpp"

enum ResourceType {
	eResource_type_Text,
	eResource_type_Binary,
	eResource_type_Image,
	eResource_type_Material,
	eResource_type_Static_Mesh,
	eResource_Type_Shader,
	eResource_type_Custom,
};

class Resource {
public:
	uint32_t LoaderID;
	const char* Name = nullptr;
	char* FullPath = nullptr;
	size_t DataSize;
	size_t DataCount;
	void* Data = nullptr;
};

struct ImageResourceData {
	unsigned char channel_count;
	uint32_t width;
	uint32_t height;
	unsigned char* pixels = nullptr;
};