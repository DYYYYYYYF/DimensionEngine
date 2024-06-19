#pragma once

#include "Defines.hpp"
#include "Containers/THashTable.hpp"
#include "Resources/ResourceTypes.hpp"

class IRenderer;

struct SShaderSystemConfig {
	unsigned short max_shader_count;
	unsigned short max_uniform_count;
	unsigned short max_global_textures;
	unsigned short max_instance_textures;
};

class ShaderSystem {
public:
	/**
	 * @brief Initializes the shader system using the supplied configuration.
	 * NOTE: Call this twice, once to obtain memory requirement (memory = 0) and a second time
	 * including allocated memory.
	 *
	 * @param renderer A pointer to Renderer frontend.
	 * @param config The configuration to be used when initializing the system.
	 * @return b8 True on success; otherwise false.
	 */
	static bool Initialize(IRenderer* renderer, SShaderSystemConfig config);

	/**
	 * @brief Shuts down the shader system.
	 */
	static void Shutdown();

	/**
	 * @brief Creates a new shader with the given config.
	 *
	 * @return True on success; otherwise false.
	 */
	static bool Create(IRenderpass* pass, ShaderConfig* config);

	/**
	 * @brief Gets the identifier of a shader by name.
	 *
	 * @param shader_name The name of the shader.
	 * @return The shader id, if found; otherwise INVALID_ID.
	 */
	static unsigned GetID(const char* shader_name);

	/**
	 * @brief Returns a pointer to a shader with the given identifier.
	 *
	 * @param shader_id The shader identifier.
	 * @return A pointer to a shader, if found; otherwise 0.
	 */
	static Shader* GetByID(uint32_t shader_id);

	/**
	 * @brief Returns a pointer to a shader with the given name.
	 *
	 * @param shader_name The name to search for. Case sensitive.
	 * @return A pointer to a shader, if found; otherwise 0.
	 */
	static Shader* Get(const char* shader_name);

	/**
	 * @brief Uses the shader with the given name.
	 *
	 * @param shader_name The name of the shader to use. Case sensitive.
	 * @return True on success; otherwise false.
	 */
	static bool Use(const char* shader_name);

	/**
	 * @brief Uses the shader with the given identifier.
	 *
	 * @param shader_id The identifier of the shader to be used.
	 * @return True on success; otherwise false.
	 */
	static bool UseByID(uint32_t shader_id);

	/**
	 * @brief Returns the uniform index for a uniform with the given name, if found.
	 *
	 * @param s A pointer to the shader to obtain the index from.
	 * @param uniform_name The name of the uniform to search for.
	 * @return The uniform index, if found; otherwise INVALID_ID_U16.
	 */
	static unsigned short GetUniformIndex(Shader* shader, const char* uniform_name);

	/**
	 * @brief Sets the value of a uniform with the given name to the supplied value.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param uniform_name The name of the uniform to be set.
	 * @param value The value to be set.
	 * @return True on success; otherwise false.
	 */
	static bool SetUniform(const char* uniform_name, const void* value);

	/**
	 * @brief Sets the texture of a sampler with the given name to the supplied texture.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param uniform_name The name of the uniform to be set.
	 * @param t A pointer to the texture to be set.
	 * @return True on success; otherwise false.
	 */
	static bool SetSampler(const char* sampler_name, const Texture* tex);

	/**
	 * @brief Sets a uniform value by index.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param index The index of the uniform.
	 * @param value The value of the uniform.
	 * @return True on success; otherwise false.
	 */
	static bool SetUniformByIndex(unsigned short index, const void* value);

	/**
	 * @brief Sets a sampler value by index.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param index The index of the uniform.
	 * @param value A pointer to the texture to be set.
	 * @return True on success; otherwise false.
	 */
	static bool SetSamplerByIndex(unsigned short index, const Texture* tex);

	/**
	 * @brief Applies global-scoped uniforms.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @return True on success; otherwise false.
	 */
	static bool ApplyGlobal();

	/**
	 * @brief Applies instance-scoped uniforms.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param need_update Indicates if the shader need uniform updated or just need to be bound.
	 * @return True on success; otherwise false.
	 */
	static bool ApplyInstance(bool need_update);

	/**
	 * @brief Binds the instance with the given id for use. Must be done before setting
	 * instance-scoped uniforms.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param instance_id The identifier of the instance to bind.
	 * @return True on success; otherwise false.
	 */
	static bool BindGlobal(uint32_t instance_id);

	/**
	 * @brief Binds the instance with the given id for use. Must be done before setting
	 * instance-scoped uniforms.
	 * NOTE: Operates against the currently-used shader.
	 *
	 * @param instance_id The identifier of the instance to bind.
	 * @return True on success; otherwise false.
	 */
	static bool BindInstance(uint32_t instance_id);

	static void Destroy(const char* shader_name);
	
private:
	static bool AddAttribute(Shader* shader, const ShaderAttributeConfig& config);
	static bool AddSampler(Shader* shader, ShaderUniformConfig& config);
	static bool AddUniform(Shader* shader, ShaderUniformConfig& config);
	static uint32_t GetShaderID(const char* shader_name);
	static uint32_t NewShaderID();
	static bool AddUniform(Shader* shader, const char* uniform_name, uint32_t size,
		ShaderUniformType type, ShaderScope scope, uint32_t set_location, bool is_sampler);
	static bool IsUniformNameValid(Shader* shader, const char* uniform_name);
	static bool IsUniformAddStateValid(Shader* shader);
	static void DestroyShader(Shader* s);

public:
	static IRenderer* Renderer;
	static SShaderSystemConfig Config;
	static HashTable Lookup;
	static void* LookupMemory;
	
	static uint32_t CurrentShaderID;
	static Shader* Shaders;
	
	static bool Initilized;
};



