#pragma once

#include "Renderer/RendererTypes.hpp"
#include <vector>

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
class IRenderbuffer;

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
	virtual void CreateTexture(const unsigned char* pixels, Texture* texture) = 0;
	virtual void DestroyTexture(Texture* txture) = 0;
	virtual void CreateWriteableTexture(Texture* tex) = 0;
	virtual void ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height) = 0;
	virtual void WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels) = 0;
	virtual void ReadTextureData(Texture* tex, uint32_t offset, uint32_t size, void** outMemeory) = 0;
	virtual void ReadTexturePixel(Texture* tex, uint32_t x, uint32_t y, unsigned char** outRGBA) = 0;

	// Geometry
	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices) = 0;
	virtual void DestroyGeometry(Geometry* geometry) = 0;
	
	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) = 0;
	virtual bool EndRenderpass(IRenderpass* pass) = 0;
	virtual bool CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) = 0;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) = 0;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass,const RenderpassConfig* config) = 0;
	virtual void DestroyRenderpass(IRenderpass* pass) = 0;
	virtual Texture* GetWindowAttachment(unsigned char index) = 0;
	virtual unsigned char GetWindowAttachmentCount() const = 0;
	virtual Texture* GetDepthAttachment(unsigned char index) = 0;
	virtual unsigned char GetWindowAttachmentIndex() = 0;

	// Renderbuffer
	virtual bool CreateRenderbuffer(enum RenderbufferType type, size_t total_size, bool use_freelist, IRenderbuffer* buffer) = 0;
	virtual bool CreateRenderbuffer(IRenderbuffer* buffer) = 0;
	virtual void DestroyRenderbuffer(IRenderbuffer* buffer) = 0;
	virtual bool BindRenderbuffer(IRenderbuffer* buffer, size_t offset) = 0;
	virtual bool UnBindRenderbuffer(IRenderbuffer* buffer) = 0;
	virtual void* MapMemory(IRenderbuffer* buffer, size_t offset, size_t size) = 0;
	virtual void UnmapMemory(IRenderbuffer* buffer, size_t offset, size_t size) = 0;
	virtual bool FlushRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size) = 0;
	virtual bool ReadRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size, void** out_memory) = 0;
	virtual bool ResizeRenderbuffer(IRenderbuffer* buffer, size_t new_size) = 0;
	virtual bool LoadRange(IRenderbuffer* buffer, size_t offset, size_t size, const void* data) = 0;
	virtual bool CopyRange(IRenderbuffer* src, size_t src_offset, IRenderbuffer* dst, size_t dst_offset, size_t size) = 0;
	virtual bool DrawRenderbuffer(IRenderbuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) = 0;

	// Render target
	virtual void SetViewport(Vec4 rect) = 0;
	virtual void ResetViewport() = 0;
	virtual void SetScissor(Vec4 rect) = 0;
	virtual void ResetScissor() = 0;

	// Shader
	virtual bool CreateShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, const std::vector<char*>& stage_filenames, std::vector<ShaderStage>& stages) = 0;
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
	virtual bool GetEnabledMultiThread() const { return false; }

public:
	SPlatformState* GetPlatformState() { return PlatformState; }

	size_t GetFrameNum() const { return FrameNum; }
	void SetFrameNum(size_t num) { FrameNum = num; }
	void IncreaseFrameNum() { FrameNum++; }

protected:
	RendererBackendType BackendType;
	SPlatformState* PlatformState = nullptr;
	size_t FrameNum;

};

