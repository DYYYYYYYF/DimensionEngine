#pragma once

#include "Rendering/Interface/IRendererBackend.hpp"
#include "VulkanContext.hpp"

class VulkanBuffer;
class VulkanShader;

class VulkanRHI : public RHI {
	friend class VulkanShader;
	friend class VulkanBuffer;
	friend class VulkanTexture;

public:
	VulkanRHI();
	virtual ~VulkanRHI();

public:
	virtual bool Initialize(const RenderBackendConfig* config, unsigned char* out_window_render_target_count, struct SPlatformState* plat_state) override;
	virtual void Shutdown() override;

	virtual bool BeginFrame() override;
	virtual void DrawGeometry(GeometryRenderData* geometry) override;
	virtual void ExecuteDrawCalls(const TArray<DrawCall>& draw_calls, size_t frame_number, const FFrameData& data) override;
	virtual bool EndFrame() override;
	virtual void Resize(unsigned short width, unsigned short height) override;

	// Textures
	virtual UTexture* AcquireTexture(const FString& name, bool auto_release) override;

	virtual bool CreateGeometry(UGeometry* geometry, const FGeometryConfig& config) override;
	virtual void DestroyGeometry(UGeometry* geometry) override;

	// Renderpass
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target) override;
	virtual bool EndRenderpass(IRenderpass* pass) override;
	virtual bool CreateRenderTarget(unsigned char attachment_count, TArray<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) override;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) override;
	virtual UTexture* GetWindowAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentCount() const override;
	virtual UTexture* GetDepthAttachment(unsigned char index) override;
	virtual unsigned char GetWindowAttachmentIndex() override;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig& config) override;
	virtual void DestroyRenderpass(IRenderpass* pass) override;

	// Renderbuffer
	virtual bool DrawRenderbuffer(IGPUBuffer* buffer, size_t offset, uint32_t element_count, bool bind_only) override;

	// Render target
	virtual void SetViewport(const Vector4& rect) override;
	virtual void ResetViewport() override;
	virtual void SetScissor(const Vector4& rect) override;
	virtual void ResetScissor() override;

	// Shaders.
	virtual bool CreateShader(UShader* shader, const FShaderConfig* config, IRenderpass* pass, const TArray<FString>& stage_filenames, TArray<ShaderStage>& stages) override;
	virtual uint32_t AcquireInstanceResource(UShader* shader, TArray<FTextureMap*>& maps) override;
	virtual bool ReleaseInstanceResource(UShader* shader, uint64_t instance_id) override;

	virtual bool AcquireTextureMap(FTextureMap* map) override;
	virtual void ReleaseTextureMap(FTextureMap* map) override;
	virtual bool GetEnabledMultiThread() const override;

public:
	virtual void CreateCommandBuffer();
	virtual bool RecreateSwapchain();

	virtual bool VerifyShaderID(uint32_t shader_id);

private:
	// 当前帧使用的 host-visible UI 合批缓冲对下标（与 ImageIndex 交替轮转）
	uint32_t CurrentUIHostBufferIndex = 0;

	// host-visible UI 合批几何重建时，跨缓冲对搬移的旧块不能立即 Free：
	// 旧块所在缓冲对的上一次命令可能仍在 GPU 执行（BeginFrame 只等待当前
	// fence 槽对应的帧，未等待对侧缓冲对的上一帧）。若立即 Free 回 freelist，
	// 后续分配会覆盖该块，导致仍在 in-flight 的 UI DrawCall 读到被篡改的
	// 顶点/索引数据（表现：层级列表时隐时现、文字贴图原图、点击后错乱）。
	// 延迟到旧块所在缓冲对再次成为写入对、BeginFrame 的 fence 等待确认其
	// in-flight 命令全部完成后统一释放。
	struct DeferredBufferFree {
		uint32_t buffer_index;  // UI 缓冲对下标 0/1
		bool bIsIndex;          // true=index 缓冲, false=vertex 缓冲
		size_t size;
		size_t offset;
	};
	TArray<DeferredBufferFree> DeferredUIHostFrees;

	void FlushDeferredUIHostFrees();

private:
	uint32_t QueryInstanceVersion();
	uint32_t SelectTargetApiVersion(uint32_t instanceVersion);

private:
	vk::SamplerAddressMode ConvertRepeatType(const char* axis, TextureRepeat repeat);
	vk::Filter ConvertFilterType(const char* op, TextureFilter filter);

protected:
	VulkanContext Context;

};