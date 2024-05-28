#pragma once

#include "RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

struct SStaticMeshData;
struct SPlatformState;
struct ShaderUniform;

class IRendererBackend;
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
	virtual void CreateTexture(Texture* texture);
	virtual void DestroyTexture(Texture* txture);

	virtual void CreateWriteableTexture(Texture* tex);
	virtual void ResizeTexture(Texture* tex, uint32_t new_width, uint32_t new_height);
	virtual void WriteTextureData(Texture* tex, uint32_t offset, uint32_t size, const unsigned char* pixels);

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices);
	virtual void DestroyGeometry(Geometry* geometry);

public:
	/**
	 * @brief Obtains a pointer of the renderpass with the given name.
	 *
	 * @param name The name of the renderpass whose identifier to obtain.
	 * @return A pointer to a renderpass if found.
	 */
	virtual IRenderpass* GetRenderpass(const char* name);

	/**
	 * @brief Creates internal shader resources using the provided parameters.
	 *
	 * @param shader A pointer to the shader.
	 * @param pass The pointer of the renderpass to be associated with the shader.
	 * @param stage_count The total number of stages.
	 * @param stage_filenames An array of shader stage filenames to be loaded. Should align with stages array.
	 * @param stages A array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
	 * @return True on success; otherwise false.
	 */
	virtual bool CreateRenderShader(Shader* shader, IRenderpass* pass, unsigned short stage_count, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages);

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
	 * @return True on success; otherwise false.
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

	virtual void CreateRenderTarget(unsigned char attachment_count, std::vector<Texture*> attachments, IRenderpass* pass, uint32_t width, uint32_t height, RenderTarget* out_target) ;
	virtual void DestroyRenderTarget(RenderTarget* target, bool free_internal_memory) ;
	virtual void CreateRenderpass(IRenderpass* out_renderpass, float depth, uint32_t stencil, bool has_prev_pass, bool has_next_pass) ;
	virtual void DestroyRenderpass(IRenderpass* pass) ;

private:
	virtual void RegenerateRenderTargets();

protected:
	RendererBackendType BackendType;
	class IRendererBackend* Backend;

	Camera* ActiveWorldCamera;

	// Projection perspective
	Matrix4 Projection;
	Matrix4 UIProjection;
	Matrix4 UIView;

	float NearClip;
	float FarClip;

	uint32_t MaterialShaderID;
	uint32_t UISHaderID;

	Vec4 AmbientColor;

	// Renderpass
	unsigned char WindowRenderTargetCount;
	uint32_t FramebufferWidth;
	uint32_t FramebufferHeight;

	IRenderpass* WorldRenderpass;
	IRenderpass* UIRenderpass;
	bool Resizing;
	unsigned char FrameSinceResize;
};
