#pragma once

#include "Math/MathTypes.hpp"
#include "Containers/FString.hpp"

#define DEFAULT_MATERIAL_NAME "Builtin.Material.Default"
#define VULKAN_MAX_MATERIAL_COUNT 1024

class Texture;

struct SMaterialConfig {
	FString name;
	FString shader_name;
	bool auto_release;
	Vector4 diffuse_color = Vector4(1.0f);
	float shininess = 16.0f;
	FString diffuse_map_name;
	FString specular_map_name;
	FString normal_map_name;

	// PBR
	float Metallic = 0.1f;					// 金属度
	float Roughness = 0.5f;					// 粗糙度
	float AmbientOcclusion = 1.0f;			// 环境光遮蔽
	Vector4 EmissiveColor;						// 自发光
	FString MetallicRoughnessTexName;	// 金属度/粗糙度Texture
	FString EmissiveFactorTexName;		// 自发光Texture

	float NormalIntensity = 1.0f;
};
