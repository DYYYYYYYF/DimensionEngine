#pragma once

#include <vector>
#include "RendererTypes.hpp"

enum ShaderStage;
struct SPlatformState;
struct ShaderUniform;

class Texture;
class Material;
class Geometry;
class Shader;
struct TextureMap;

enum BuiltinRenderpass : unsigned char{
	eButilin_Renderpass_World = 0x01,
	eButilin_Renderpass_UI = 0x02
};

class IRendererBackend {
public:
	IRendererBackend() {};

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state) = 0;
	virtual void Shutdown() = 0;

	virtual bool BeginFrame(double delta_time) = 0;
	virtual bool EndFrame(double delta_time) = 0;
	virtual void Resize(unsigned short width, unsigned short height) = 0;

	virtual void DrawGeometry(GeometryRenderData geometry) = 0;

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) = 0;
	virtual void DestroyTexture(Texture* txture) = 0;

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) = 0;
	virtual void DestroyGeometry(Geometry* geometry) = 0;
	
	virtual bool BeginRenderpass(unsigned char renderpass_id) = 0;
	virtual bool EndRenderpass(unsigned char renderpass_id) = 0;

public:
	virtual bool CreateShader(Shader* shader, unsigned short renderpass_id, unsigned short stage_count, const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) = 0;
	virtual bool DestroyShader(Shader* shader) = 0;
	virtual bool InitializeShader(Shader* shader) = 0;
	virtual bool UseShader(Shader* shader) = 0;
	virtual bool BindGlobalsShader(Shader* shader) = 0;
	virtual bool BindInstanceShader(Shader* shader, uint32_t instance_id) = 0;
	virtual bool ApplyGlobalShader(Shader* shader) = 0;
	virtual bool ApplyInstanceShader(Shader* shader, bool need_update) = 0;
	virtual uint32_t AcquireInstanceResource(Shader* shader, std::vector<TextureMap*>& maps) = 0;
	virtual bool ReleaseInstanceResource(Shader* shader, uint32_t instance_id) = 0;
	virtual bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value) = 0;

	virtual bool AcquireTextureMap(TextureMap* map) = 0;
	virtual void ReleaseTextureMap(TextureMap* map) = 0;

public:
	SPlatformState* GetPlatformState() { return PlatformState; }

	size_t GetFrameNum() const { return FrameNum; }
	void SetFrameNum(size_t num) { FrameNum = num; }
	void IncreaseFrameNum() { FrameNum++; }

public:
	// Points to default textures.
	Texture* DefaultDiffuse = nullptr;

protected:
	RendererBackendType BackendType;
	SPlatformState* PlatformState;
	size_t FrameNum;

};

