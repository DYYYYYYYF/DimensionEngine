#pragma once

#include "Rendering/RenderTypes.hpp"

enum ShaderStage;
struct SPlatformState;
struct ShaderUniform;
struct GeometryRenderData;
struct RenderTarget;
struct TextureMap;
struct RenderBackendConfig;
struct ShaderConfig;

class Texture;
class Material;
class Geometry;
class Shader;
class IRenderpass;
class IGPUBuffer;

enum BuiltinRenderpass : unsigned char{
	eButilin_Renderpass_World = 0x01,
	eButilin_Renderpass_UI = 0x02
};

class IRendererBackend {
public:
	// Generic
	virtual bool Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, SPlatformState* plat_state) = 0;
	virtual void Shutdown() = 0;
	virtual bool BeginFrame(double delta_time) = 0;
	virtual bool EndFrame(double delta_time) = 0;
	virtual void Resize(unsigned short width, unsigned short height) = 0;
	virtual void DrawGeometry(GeometryRenderData* geometry) = 0;

	// Texture
	virtual UTexture* AcquireTexture(const FString& name, bool auto_release) = 0;

	// Geometry
	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) = 0;
	virtual void DestroyGeometry(Geometry* geometry) = 0;
	
	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) = 0;
	virtual bool EndRenderpass(IRenderpass* pass) = 0;
	virtual bool CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) = 0;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) = 0;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass,const RenderpassConfig& config) = 0;
	virtual void DestroyRenderpass(IRenderpass* pass) = 0;
	virtual UTexture* GetWindowAttachment(unsigned char index) = 0;
	virtual unsigned char GetWindowAttachmentCount() const = 0;
	virtual UTexture* GetDepthAttachment(unsigned char index) = 0;
	virtual unsigned char GetWindowAttachmentIndex() = 0;

	// Renderbuffer
	virtual bool DrawRenderbuffer(IGPUBuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) = 0;

	// Render target
	virtual void SetViewport(const Vector4& rect) = 0;
	virtual void ResetViewport() = 0;
	virtual void SetScissor(const Vector4& rect) = 0;
	virtual void ResetScissor() = 0;

	// Shader
	virtual bool CreateShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, const TArray<FString>& stage_filenames, std::vector<ShaderStage>& stages) = 0;
	virtual uint32_t AcquireInstanceResource(Shader* shader, std::vector<TextureMap*>& maps) = 0;
	virtual bool ReleaseInstanceResource(Shader* shader, uint64_t instance_id) = 0;

	virtual bool AcquireTextureMap(TextureMap* map) = 0;
	virtual void ReleaseTextureMap(TextureMap* map) = 0;
	virtual bool GetEnabledMultiThread() const { return false; }

public:
	SPlatformState* GetPlatformState() { return PlatformState; }

	size_t GetFrameNum() const { return FrameNum; }
	void SetFrameNum(size_t num) { FrameNum = num; }
	void IncreaseFrameNum() { FrameNum++; }

protected:
	RendererBackendType BackendType = RendererBackendType::eRenderer_Backend_Type_Vulkan;
	SPlatformState* PlatformState = nullptr;
	size_t FrameNum = 0;

};

