#pragma once

#include "Math/MathTypes.hpp"
#include "Resources/Texture.hpp"

#define DEFAULT_MATERIAL_NAME "default"
#define VULKAN_MAX_MATERIAL_COUNT 1024
#define MATERIAL_NAME_MAX_LENGTH 256

class Texture;

struct SMaterialConfig {
	char name[MATERIAL_NAME_MAX_LENGTH];
	const char* shader_name = nullptr;
	bool auto_release;
	Vec4 diffuse_color;
	float shininess;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
	char specular_map_name[TEXTURE_NAME_MAX_LENGTH];
	char normal_map_name[TEXTURE_NAME_MAX_LENGTH];
};

class Material {
public:
	Material() {}

public:
	uint32_t Id;
	uint32_t Generation;
	uint32_t InternalId;
	char Name[MATERIAL_NAME_MAX_LENGTH];
	Vec4 DiffuseColor;
	TextureMap DiffuseMap;
	TextureMap SpecularMap;
	TextureMap NormalMap;
	float Shininess;

	uint32_t ShaderID;
	uint32_t RenderFrameNumer;
};
