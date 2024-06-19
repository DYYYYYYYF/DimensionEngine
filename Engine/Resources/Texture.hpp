#pragma once

#include "Math/MathTypes.hpp"

#define TEXTURE_NAME_MAX_LENGTH 512

enum TextureUsage {
	eTexture_Usage_Unknown = 0x00,
	eTexture_Usage_Map_Diffuse = 0x01,
	eTexture_Usage_Map_Specular = 0x02,
	eTexture_Usage_Map_Normal = 0x03,
	eTexture_Usage_Map_Cubemap = 0x04
};

enum TextureFilter {
	eTexture_Filter_Mode_Nearest = 0x0,
	eTexture_Filter_Mode_Linear = 0x1
};

enum TextureRepeat{
	eTexture_Repeat_Repeat = 0x0,
	eTexture_Repeat_Minrrored_Repeat = 0x1,
	eTexture_Repeat_Clamp_To_Edge = 0x2,
	eTexture_Repeat_Clamp_To_Border = 0x3,
};

enum TextureFlagBits{
	eTexture_Flag_Has_Transparency = 0x1,
	eTexture_Flag_Is_Writeable = 0x2,
	eTexture_Flag_Is_Wrapped = 0x4,
	eTexture_Flag_Depth = 0x8
};
typedef unsigned char TextureFlag;

enum TextureType {
	eTexture_Type_2D,
	eTexture_Type_Cube
};

struct TextureMap {
	class Texture* texture = nullptr;
	TextureUsage usage;
	TextureFilter filter_minify;
	TextureFilter filter_magnify;
	TextureRepeat repeat_u;
	TextureRepeat repeat_v;
	TextureRepeat repeat_w;
	void* internal_data = nullptr;
};

class Texture {
public:
	Texture() {}
	virtual ~Texture() {}

public:
	uint32_t Id;
	TextureType Type;
	uint32_t Width;
	uint32_t Height;

	int ChannelCount;
	/** TextureFlag */
	TextureFlag Flags = 0;

	uint32_t Generation;
	char Name[TEXTURE_NAME_MAX_LENGTH];
	void* InternalData = nullptr;
};

class Skybox {
public:
	TextureMap Cubemap;
	class Geometry* g = nullptr;
	uint32_t InstanceID;
	size_t RenderFrameNumber;
};
