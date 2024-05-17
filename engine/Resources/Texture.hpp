#pragma once

#include "Math/MathTypes.hpp"

#define TEXTURE_NAME_MAX_LENGTH 512

enum TextureUsage {
	eTexture_Usage_Unknown = 0x00,
	eTexture_Usage_Map_Diffuse = 0x01,
	eTexture_Usage_Map_Specular = 0x02,
	eTexture_Usage_Map_Normal = 0x03
};

class Texture {
public:
	Texture() {}
	virtual ~Texture() {}

public:
	uint32_t Id;
	uint32_t Width;
	uint32_t Height;

	int ChannelCount;
	bool HasTransparency;

	uint32_t Generation;
	char Name[TEXTURE_NAME_MAX_LENGTH];
	void* InternalData = nullptr;
};