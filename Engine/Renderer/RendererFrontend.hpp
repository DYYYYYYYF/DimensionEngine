#pragma once

#include "RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

struct SStaticMeshData;
struct SPlatformState;
struct ShaderUniform;

class IRendererBackend;
class IRenderbuffer;
class IRenderpass;
class Geometry;
class Shader;
class Camera;

class IRenderer {
public:
	IRenderer();
	IRenderer(RendererBackendType type, struct SPlatformState* plat_state);
	virtual ~IRenderer();

public:
	virtual bool Initialize(const char* application_name, struct SPlatformState* plat_state);
	virtual void Shutdown();

	virtual void OnResize(unsigned short width, unsigned short height);
	virtual bool DrawFrame(SRenderPacket* packet);

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture);
	virtual void DestroyTexture(Texture* texture);

	virtual void CreateWriteableTexture(Texture* tex);
	virtual void ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height);
	virtual void WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels);

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices);
	virtual void DestroyGeometry(Geometry* geometry);

	virtual bool GetEnabledMutiThread() const;

public:
	/**
	 * @beief Draws the given geometry. Should only be called inside a renderpas, within a frame.
	 * 
	 * @param data The render data of the geometry to be drawn.
	 */
	virtual void DrawGeometry(GeometryRenderData* data);

	/**
	 * @brief Begins the given renderpass.
	 * 
	 * @param pass A pointer to the renderpass to begin.
	 * @param target A pointer to the render target to be used.
	 * @return True on success.
	 */
	virtual bool BeginRenderpass(IRenderpass* pass, RenderTarget* target);

	/**
	 * @beief End the given renderpass.
	 * 
	 * @param pass A pointer to the renderpass to begin.
	 * @return True on success.
	 */
	virtual bool EndRenderpass(IRenderpass* pass);

	/**
	 * @brief Creates internal shader resources using the provided parameters.
	 *
	 * @param shader A pointer to the shader.
	 * @param pass The pointer of the renderpass to be associated with the shader.
	 * @param stage_filenames An array of shader stage filenames to be loaded. Should align with stages array.
	 * @param stages A array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
	 * @return True on success; otherwise false.
	 */
	virtual bool CreateRenderShader(Shader* shader, const ShaderConfig* config, IRenderpass* pass, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages);

	/**
	 * @brief Destroys the given shader and releases any resources held by it.
	 * @param shader A pointer to the shader to be destroyed.
	 */
	virtual bool DestroyRenderShader(Shader* shader);

	/**
	 * @brief Initializes a configured shader. Will be automatically destroyed if this step fails.
	 * Must be done after vulkan_shader_create().
	 *
	 * @param shader A pointer to the shader to be initialized.
	 * @return True on success; otherwise false.
	 */
	virtual bool InitializeRenderShader(Shader* shader);

	/**
	 * @brief Uses the given shader, activating it for updates to attributes, uniforms and such,
	 * and for use in draw calls.
	 *
	 * @param shader A pointer to the shader to be used.
	 * @return True on success; otherwise false.
	 */
	virtual bool UseRenderShader(Shader* shader);

	/**
	 * @brief Binds global resources for use and updating.
	 *
	 * @param shader A pointer to the shader whose globals are to be bound.
	 * @return True on success; otherwise false.
	 */
	virtual bool BindGlobalsRenderShader(Shader* shader);

	/**
	 * @brief Binds instance resources for use and updating.
	 *
	 * @param shader A pointer to the shader whose instance resources are to be bound.
	 * @param instance_id The identifier of the instance to be bound.
	 * @return True on success; otherwise false.
	 */
	virtual bool BindInstanceRenderShader(Shader* shader, uint32_t instance_id);

	/**
	 * @brief Applies global data to the uniform buffer.
	 *
	 * @param shader A pointer to the shader to apply the global data for.
	 * @return True on success; otherwise false.
	 */
	virtual bool ApplyGlobalRenderShader(Shader* shader);

	/**
	 * @brief Applies data for the currently bound instance.
	 *
	 * @param shader A pointer to the shader to apply the instance data for.
	 * @param need_update
	 * @return True on success; otherwise false.
	 */
	virtual bool ApplyInstanceRenderShader(Shader* shader, bool need_update);

	/**
	 * @brief Acquires internal instance-level resources and provides an instance id.
	 *
	 * @param shader A pointer to the shader to acquire resources from.
	 * @param maps Array to hold the texture maps.
	 * @return INVALID_ID on false; otherwise return the instance id.
	 */
	virtual uint32_t AcquireInstanceResource(Shader* shader, std::vector<TextureMap*> maps);

	/**
	 * @brief Releases internal instance-level resources for the given instance id.
	 *
	 * @param shader A pointer to the shader to release resources from.
	 * @param instance_id The instance identifier whose resources are to be released.
	 * @return True on success; otherwise false.
	 */
	virtual bool ReleaseInstanceResource(Shader* shader, uint32_t instance_id);

	/**
	 * @brief Sets the uniform of the given shader to the provided value.
	 *
	 * @param shader A pointer to the shader.
	 * @param uniform A constant pointer to the uniform.
	 * @param value A pointer to the value to be set.
	 * @return b8 True on success; otherwise false.
	 */
	virtual bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value);

	/**
	 * @brief Acquires internal resource for the given texture map.
	 * 
	 * @param map A pointer to texture map to obtain resources for.
	 * @return True on success.
	 */
	virtual bool AcquireTextureMap(TextureMap* map);
	
	/**
	 * @brief Release internal resource for the given texture map.
	 *
	 * @param map A pointer to texture map to obtain resources for.
	 */
	virtual void ReleaseTextureMap(TextureMap* map);

	virtual void ReadTextureData(Texture* tex, uint32_t offset, uint32_t size, void** outMemeory);
	virtual void ReadTexturePixel(Texture* tex, uint32_t x, uint32_t y, unsigned char** outRGBA);

	// Renderbuffer
	virtual bool CreateRenderbuffer(enum RenderbufferType type, size_t total_size, bool use_freelist, IRenderbuffer* buffer);
	virtual void DestroyRenderbuffer(IRenderbuffer* buffer);
	virtual bool BindRenderbuffer(IRenderbuffer* buffer, size_t offset);
	virtual bool UnBindRenderbuffer(IRenderbuffer* buffer);
	virtual void* MapMemory(IRenderbuffer* buffer, size_t offset, size_t size);
	virtual void UnmapMemory(IRenderbuffer* buffer, size_t offset, size_t size);
	virtual bool FlushRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size);
	virtual bool ReadRenderbuffer(IRenderbuffer* buffer, size_t offset, size_t size, void** out_memory);
	virtual bool ResizeRenderbuffer(IRenderbuffer* buffer, size_t new_size);
	virtual bool LoadRange(IRenderbuffer* buffer, size_t offset, size_t size, const void* data);
	virtual bool CopyRange(IRenderbuffer* src, size_t src_offset, IRenderbuffer* dst, size_t dst_offset, size_t size);
	virtual bool DrawRenderbuffer(IRenderbuffer* buffer, size_t offset, uint32_t element_count, bool bind_only);
	virtual bool AllocateRenderbuffer(IRenderbuffer* buffer, size_t size, size_t* out_offset);
	virtual bool FreeRenderbuffer(IRenderbuffer* buffer, size_t size, size_t offset);
	
	// Render target
	virtual void SetViewport(Vec4 rect);
	virtual void ResetViewport();
	virtual void SetScissor(Vec4 rect);
	virtual void ResetScissor();

	// Renderpass
	virtual bool CreateRenderTarget(unsigned char attachment_count, std::vector<RenderTargetAttachment> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target);
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) ;
	virtual bool CreateRenderpass(IRenderpass* out_renderpass, const RenderpassConfig* config);
	virtual void DestroyRenderpass(IRenderpass* pass) ;
	virtual IRendererBackend* GetRenderBackend() { return Backend; }

	virtual Texture* GetWindowAttachment(unsigned char index);
	virtual unsigned char GetWindowAttachmentCount() const;
	virtual Texture* GetDepthAttachment(unsigned char index);
	virtual unsigned char GetWindowAttachmentIndex();

public:
	RendererBackendType GetBackendType() const { return BackendType; }

protected:
	RendererBackendType BackendType;
	class IRendererBackend* Backend = nullptr;

	unsigned char WindowRenderTargetCount;
	uint32_t FramebufferWidth;
	uint32_t FramebufferHeight;

	bool Resizing;
	unsigned char FrameSinceResize;
};
