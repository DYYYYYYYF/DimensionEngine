#pragma once

#include "RendererTypes.hpp"
#include "Resources/ResourceTypes.hpp"

struct SStaticMeshData;
struct SPlatformState;
struct ShaderUniform;

class IRendererBackend;
class Geometry;
class Shader;

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
	
	virtual void SetViewTransform(Matrix4 view) { View = view; }

	virtual void CreateTexture(const unsigned char* pixels, Texture* texture);
	virtual void CreateTexture(Texture* texture);
	virtual void DestroyTexture(Texture* txture);

	virtual bool CreateGeometry(Geometry* geometry, uint32_t vertex_size, uint32_t vertex_count, 
		const void* vertices, uint32_t index_size, uint32_t index_count, const void* indices);
	virtual void DestroyGeometry(Geometry* geometry);

public:
	/**
	 * @brief Obtains the identifier of the renderpass with the given name.
	 *
	 * @param name The name of the renderpass whose identifier to obtain.
	 * @return INVALID_ID_U16 if not found; otherwise id.
	 */
	virtual unsigned short GetRenderpassID(const char* name);

	/**
	 * @brief Creates internal shader resources using the provided parameters.
	 *
	 * @param shader A pointer to the shader.
	 * @param renderpass_id The identifier of the renderpass to be associated with the shader.
	 * @param stage_count The total number of stages.
	 * @param stage_filenames An array of shader stage filenames to be loaded. Should align with stages array.
	 * @param stages A array of shader_stages indicating what render stages (vertex, fragment, etc.) used in this shader.
	 * @return True on success; otherwise false.
	 */
	virtual bool CreateRenderShader(Shader* shader, unsigned short renderpass_id, unsigned short stage_count, std::vector<char*> stage_filenames, std::vector<ShaderStage> stages);

	/**
	 * @brief Destroys the given shader and releases any resources held by it.
	 * @param s A pointer to the shader to be destroyed.
	 */
	virtual bool DestroyRenderShader(Shader* shader);

	/**
	 * @brief Initializes a configured shader. Will be automatically destroyed if this step fails.
	 * Must be done after vulkan_shader_create().
	 *
	 * @param s A pointer to the shader to be initialized.
	 * @return True on success; otherwise false.
	 */
	virtual bool InitializeRenderShader(Shader* shader);

	/**
	 * @brief Uses the given shader, activating it for updates to attributes, uniforms and such,
	 * and for use in draw calls.
	 *
	 * @param s A pointer to the shader to be used.
	 * @return True on success; otherwise false.
	 */
	virtual bool UseRenderShader(Shader* shader);

	/**
	 * @brief Binds global resources for use and updating.
	 *
	 * @param s A pointer to the shader whose globals are to be bound.
	 * @return True on success; otherwise false.
	 */
	virtual bool BindGlobalsRenderShader(Shader* shader);

	/**
	 * @brief Binds instance resources for use and updating.
	 *
	 * @param s A pointer to the shader whose instance resources are to be bound.
	 * @param instance_id The identifier of the instance to be bound.
	 * @return True on success; otherwise false.
	 */
	virtual bool BindInstanceRenderShader(Shader* shader, uint32_t instance_id);

	/**
	 * @brief Applies global data to the uniform buffer.
	 *
	 * @param s A pointer to the shader to apply the global data for.
	 * @return True on success; otherwise false.
	 */
	virtual bool ApplyGlobalRenderShader(Shader* shader);

	/**
	 * @brief Applies data for the currently bound instance.
	 *
	 * @param s A pointer to the shader to apply the instance data for.
	 * @return True on success; otherwise false.
	 */
	virtual bool ApplyInstanceRenderShader(Shader* shader);

	/**
	 * @brief Acquires internal instance-level resources and provides an instance id.
	 *
	 * @param s A pointer to the shader to acquire resources from.
	 * @param out_instance_id A pointer to hold the new instance identifier.
	 * @return True on success; otherwise false.
	 */
	virtual uint32_t AcquireInstanceResource(Shader* shader);

	/**
	 * @brief Releases internal instance-level resources for the given instance id.
	 *
	 * @param s A pointer to the shader to release resources from.
	 * @param instance_id The instance identifier whose resources are to be released.
	 * @return True on success; otherwise false.
	 */
	virtual bool ReleaseInstanceResource(Shader* shader, uint32_t instance_id);

	/**
	 * @brief Sets the uniform of the given shader to the provided value.
	 *
	 * @param s A ponter to the shader.
	 * @param uniform A constant pointer to the uniform.
	 * @param value A pointer to the value to be set.
	 * @return b8 True on success; otherwise false.
	 */
	virtual bool SetUniform(Shader* shader, ShaderUniform* uniform, const void* value);

protected:
	RendererBackendType BackendType;
	class IRendererBackend* Backend;

	// Projection perspective
	Matrix4 Projection;
	Matrix4 View;
	float NearClip;
	float FarClip;

	// Material Shader
	uint32_t MaterialShaderID;
	uint32_t MaterialShaderProjectionLocation;
	uint32_t MaterialShaderViewLocation;
	uint32_t MaterialShaderDiffuseColorLocation;
	uint32_t MaterialShaderDiffuseTextureLocation;
	uint32_t MaterialShaderModelLocation;

	// UI Shader
	uint32_t UIShaderID;
	uint32_t UIShaderProjectionLocation;
	uint32_t UIShaderViewLocation;
	uint32_t UIShaderDiffuseColorLocation;
	uint32_t UIShaderDiffuseTextureLocation;
	uint32_t UIShaderModelLocation;

};
