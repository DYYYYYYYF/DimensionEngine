#pragma once

#include <vulkan/vulkan.hpp>
#include "Containers/TArray.hpp"
#include "Containers/THashTable.hpp"
#include "Math/MathTypes.hpp"

class Texture;

#define VULKAN_SHADER_MAX_STAGES 8
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
#define VULKAN_SHADER_MAX_ATTRIBUTES 16

#define VULKAN_SHADER_MAX_UNIFORMS 128
#define VULKAN_SHADER_MAX_BINDINGS 32
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

enum ShaderState {
	eShader_State_Not_Created,
	eShader_State_Uninitialized,
	eShader_State_Initialized,
};

enum ShaderStage {
	eShader_Stage_Vertex = 0x00000001,
	eShader_Stage_Geometry = 0x00000002,
	eShader_Stage_Fragment = 0x00000004,
	eShader_Stage_Compute = 0x00000008
};

/*
enum ShaderAttributeType {
	eShader_Attribute_Type_Float,
	eShader_Attribute_Type_Float_2,
	eShader_Attribute_Type_Float_3,
	eShader_Attribute_Type_Float_4,
	eShader_Attribute_Type_Matrix,
	eShader_Attribute_Type_Int8,
	eShader_Attribute_Type_Int8_2,
	eShader_Attribute_Type_Int8_3,
	eShader_Attribute_Type_Int8_4,
	eShader_Attribute_Type_UInt8,
	eShader_Attribute_Type_UInt8_2,
	eShader_Attribute_Type_UInt8_3,
	eShader_Attribute_Type_UInt8_4,
	eShader_Attribute_Type_Int16,
	eShader_Attribute_Type_Int16_2,
	eShader_Attribute_Type_Int16_3,
	eShader_Attribute_Type_Int16_4,
	eShader_Attribute_Type_UInt16,
	eShader_Attribute_Type_UInt16_2,
	eShader_Attribute_Type_UInt16_3,
	eShader_Attribute_Type_UInt16_4,
	eShader_Attribute_Type_Int32,
	eShader_Attribute_Type_Int32_2,
	eShader_Attribute_Type_Int32_3,
	eShader_Attribute_Type_Int32_4,
	eShader_Attribute_Type_UInt32,
	eShader_Attribute_Type_UInt32_2,
	eShader_Attribute_Type_UInt32_3,
	eShader_Attribute_Type_UInt32_4,
};
*/

enum ShaderAttributeType {
	eShader_Attribute_Type_Float	= 0U,
	eShader_Attribute_Type_Float_2	= 1U,
	eShader_Attribute_Type_Float_3	= 2U,
	eShader_Attribute_Type_Float_4	= 3U,
	eShader_Attribute_Type_Matrix	= 4U,
	eShader_Attribute_Type_Int8		= 5U,
	eShader_Attribute_Type_UInt8	= 6U,
	eShader_Attribute_Type_Int16	= 7U,
	eShader_Attribute_Type_UInt16	= 8U,
	eShader_Attribute_Type_Int32	= 9U,
	eShader_Attribute_Type_UInt32	= 10U
};

enum ShaderUniformType {
	eShader_Uniform_Type_Float		= 0U,
	eShader_Uniform_Type_Float_2	= 1U,
	eShader_Uniform_Type_Float_3	= 2U,
	eShader_Uniform_Type_Float_4	= 3U,
	eShader_Uniform_Type_Matrix		= 4U,
	eShader_Uniform_Type_Int8		= 5U,
	eShader_Uniform_Type_UInt8		= 6U,
	eShader_Uniform_Type_Int16		= 7U,
	eShader_Uniform_Type_UInt16		= 8U,
	eShader_Uniform_Type_Int32		= 9U,
	eShader_Uniform_Type_UInt32		= 10U,
	eShader_Uniform_Type_Sampler	= 11U,
	eShader_Uniform_Type_Custom		= 12U
};

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated.
 */
enum ShaderScope {
	eShader_Scope_Global = 0,
	eShader_Scope_Instance = 1,
	eShader_Scope_Local = 2
};

struct ShaderUniform {
	size_t offset;
	unsigned short location;
	unsigned short index;
	unsigned short size;
	unsigned short set_index;

	ShaderScope scope;
	ShaderUniformType type;
};

struct ShaderAttribute {
	char* name = nullptr;
	ShaderAttributeType type;
	uint32_t size;
};

struct ShaderAttributeConfig {
	unsigned short name_length;
	char* name = nullptr;
	unsigned short size;
	uint32_t location;
	ShaderAttributeType type;
};

struct ShaderUniformConfig {
	unsigned short name_length;
	char* name = nullptr;
	unsigned short size;
	uint32_t location;
	ShaderUniformType type;
	ShaderScope scope;
};

// Uniform Buffer Object
struct MaterialShaderGlobalUbo {
	Matrix4 projection;	// 64 bytes
	Matrix4 view;		// 64 bytes
	Matrix4 reserved0;	// 64 bytes, reserved for future use
	Matrix4 reserved1;	// 64 bytes, reserved for future use
};

// Object Material
struct MaterialShaderInstanceUbo {
	Vec4 diffuse_color;	// 16 Bytes
	Vec4 v_reserved0;	// 16 Bytes,reserved for future use
	Vec4 v_reserved1;	// 16 Bytes,reserved for future use
	Vec4 v_reserved2;	// 16 Bytes,reserved for future use
};

struct VulkanDescriptorState {
	// One per frame
	uint32_t generations[3];
	uint32_t ids[3];
};

struct ShaderConfig {
	char* name;
	bool use_instances;
	bool use_local;

	unsigned short attribute_count;
	std::vector<ShaderAttributeConfig> attributes;

	unsigned short uniform_count;
	std::vector<ShaderUniformConfig> uniforms;

	char* renderpass_name;

	unsigned short stage_cout;
	std::vector<ShaderStage> stages;

	std::vector<char*> stage_names;
	std::vector<char*> stage_filenames;
};

class Shader {
public:
	uint32_t ID;
	char* Name;
	bool UseInstances;
	bool UseLocals;

	size_t RequiredUboAlignment;
	size_t GlobalUboSize;
	size_t GlobalUboOffset;
	size_t GlobalUboStride;
	size_t UboSize;
	size_t UboStride;
	size_t PushConstantsSize;
	size_t PushConstantsStride;
	uint32_t InstanceTextureCount;
	ShaderScope BoundScope;
	uint32_t BoundInstanceId;
	uint32_t BoundUboOffset;
	void* HashtableBlock;
	HashTable UniformLookup;
	ShaderState State;
	unsigned short PushConstantsRangeCount;
	Range PushConstantsRanges[32];
	unsigned short AttributeStride;
	void* InternalData;

	std::vector<ShaderUniform> Uniforms;
	std::vector<ShaderAttribute> Attributes;
	std::vector<Texture*> GlobalTextures;
};