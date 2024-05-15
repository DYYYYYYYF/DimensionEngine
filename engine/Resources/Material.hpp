#pragma once

#include "Math/MathTypes.hpp"

#define MATERIAL_NAME_MAX_LENGTH 256

class Texture;

struct SMaterialConfig {
	char name[MATERIAL_NAME_MAX_LENGTH];
	char* shader_name;
	bool auto_release;
	Vec4 diffuse_color;
	char diffuse_map_name[TEXTURE_NAME_MAX_LENGTH];
};

struct MaterialShaderUniformLocations {
	unsigned short projection;
	unsigned short view;
	unsigned short diffuse_color;
	unsigned short diffuse_texture;
	unsigned short model;
};

struct UIShaderUniformLocations {
	unsigned short projection;
	unsigned short view;
	unsigned short diffuse_color;
	unsigned short diffuse_texture;
	unsigned short model;
};

struct TextureMap {
	Texture* texture;
	TextureUsage usage;
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

	uint32_t ShaderID;
};