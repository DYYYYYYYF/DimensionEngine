#pragma once

#include "Math/MathTypes.hpp"

enum class EShaderLanguage {
	eHLSL,
	eGLSL
};

enum class EShaderRenderMode {
	eShader_Render_Mode_Default,
	eShader_Render_Mode_Lighting,
	eShader_Render_Mode_Normals,
	eShader_Render_Mode_Depth
};

enum class EShaderStatus {
	eShader_State_Not_Created,
	eShader_State_Uninitialized,
	eShader_State_Initialized,
	eShader_State_Reloading
};

enum ShaderStage {
	eShader_Stage_Vertex = 0x00000001,
	eShader_Stage_Geometry = 0x00000002,
	eShader_Stage_Fragment = 0x00000004,
	eShader_Stage_Compute = 0x00000008
};

enum ShaderAttributeType {
	eShader_Attribute_Type_Float = 0U,
	eShader_Attribute_Type_Float_2 = 1U,
	eShader_Attribute_Type_Float_3 = 2U,
	eShader_Attribute_Type_Float_4 = 3U,
	eShader_Attribute_Type_Matrix = 4U,
	eShader_Attribute_Type_Int8 = 5U,
	eShader_Attribute_Type_UInt8 = 6U,
	eShader_Attribute_Type_Int16 = 7U,
	eShader_Attribute_Type_UInt16 = 8U,
	eShader_Attribute_Type_Int32 = 9U,
	eShader_Attribute_Type_UInt32 = 10U
};

enum ShaderUniformType {
	eShader_Uniform_Type_Float = 0U,
	eShader_Uniform_Type_Float_2 = 1U,
	eShader_Uniform_Type_Float_3 = 2U,
	eShader_Uniform_Type_Float_4 = 3U,
	eShader_Uniform_Type_Matrix = 4U,
	eShader_Uniform_Type_Int8 = 5U,
	eShader_Uniform_Type_UInt8 = 6U,
	eShader_Uniform_Type_Int16 = 7U,
	eShader_Uniform_Type_UInt16 = 8U,
	eShader_Uniform_Type_Int32 = 9U,
	eShader_Uniform_Type_UInt32 = 10U,
	eShader_Uniform_Type_Sampler = 11U,
	eShader_Uniform_Type_Custom = 12U
};

enum FaceCullMode {
	eFace_Cull_Mode_None = 0x0,
	eFace_Cull_Mode_Front = 0x1,
	eFace_Cull_Mode_Back = 0x2,
	eFace_Cull_Mode_Front_And_Back = 0x3,
};

enum class PrimitiveTopology {
	eTriangleList = 0x00,
	eTriangleStrip = 0x01,
};

enum PolygonMode {
	ePology_Mode_Fill = 0x0,
	ePology_Mode_Line = 0x1,
};

enum ShaderFlags {
	eShader_Flag_None = 0x0,
	eShader_Flag_DepthTest = 0x1,
	eShader_Flag_DepthWrite = 0x2,
};

typedef unsigned int ShaderFlagBits;

struct MaterialShaderUniformLocations {
	uint32_t projection = INVALID_ID;
	uint32_t view = INVALID_ID;
	uint32_t ambient_color = INVALID_ID;
	uint32_t view_position = INVALID_ID;
	uint32_t model = INVALID_ID;
	uint32_t time = INVALID_ID;
	uint32_t diffuse_color = INVALID_ID;
	uint32_t shininess = INVALID_ID;
	uint32_t metallic = INVALID_ID;
	uint32_t roughness = INVALID_ID;
	uint32_t ambient_occlusion = INVALID_ID;
	uint32_t normal_intensity = INVALID_ID;

	uint32_t diffuse_texture = INVALID_ID;
	uint32_t specular_texture = INVALID_ID;
	uint32_t normal_texture = INVALID_ID;
	uint32_t roughness_metallic_texture = INVALID_ID;

	uint32_t render_mode = INVALID_ID;
};

struct DRShaderUniformLocations {
	uint32_t projection = INVALID_ID;
	uint32_t view = INVALID_ID;
	uint32_t ambient_color = INVALID_ID;
	uint32_t view_position = INVALID_ID;
	uint32_t mode = INVALID_ID;
	uint32_t time = INVALID_ID;
	uint32_t albedo_texture = INVALID_ID;
	uint32_t normal_texture = INVALID_ID;
	uint32_t position_texture = INVALID_ID;
	uint32_t light_intensity = INVALID_ID;
	uint32_t debug_mode = INVALID_ID;
};

struct UIShaderUniformLocations {
	uint32_t projection = INVALID_ID_U16;
	uint32_t view = INVALID_ID_U16;
	uint32_t diffuse_color = INVALID_ID_U16;
	uint32_t diffuse_texture = INVALID_ID_U16;
	uint32_t model = INVALID_ID_U16;
};

/**
 * @brief Defines shader scope, which indicates how
 * often it gets updated
 */
enum ShaderScope {
	eShader_Scope_Global = 0,
	eShader_Scope_Instance = 1,
	eShader_Scope_Local = 2
};

struct ShaderUniform {
	size_t offset;
	uint32_t location;
	uint32_t index;
	uint32_t size;
	uint32_t set_index;

	ShaderScope scope;
	ShaderUniformType type;
};

struct ShaderAttribute {
	char* name = nullptr;
	uint32_t size;
	ShaderAttributeType type;
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
	Vector4 diffuse_color;	// 16 Bytes
	Vector4 v_reserved0;	// 16 Bytes,reserved for future use
	Vector4 v_reserved1;	// 16 Bytes,reserved for future use
	Vector4 v_reserved2;	// 16 Bytes,reserved for future use
};

struct ShaderConfig {
public:
	ShaderConfig() {
		name = nullptr;
		cull_mode = FaceCullMode::eFace_Cull_Mode_Back;
		polygon_mode = PolygonMode::ePology_Mode_Fill;
		PrimTopo = PrimitiveTopology::eTriangleList;
		depthTest = true;
		depthWrite = true;
	}

	char* name = nullptr;
	float time = 0.0f;
	FaceCullMode cull_mode;
	PolygonMode polygon_mode;
	PrimitiveTopology PrimTopo;

	std::vector<ShaderAttributeConfig> attributes;
	std::vector<ShaderUniformConfig> uniforms;
	std::vector<ShaderStage> stages;
	std::vector<char*> stage_names;
	std::vector<char*> stage_filenames;

	bool depthTest;
	bool depthWrite;
};
