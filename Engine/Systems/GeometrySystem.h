#pragma once

#include "MaterialSystem.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"

#define GEOMETRY_MAX_COUNT 4096
#define DEFAULT_GEOMETRY_PLANE_NAME "DefaultGeometryPlane"
#define DEFAULT_GEOMETRY_CUBE_NAME "DefaultGeometryCube"

struct GeometryData {
	uint32_t id = INVALID_ID;
	uint32_t generation = INVALID_ID;
	// Vertices
	uint32_t vertex_count = 0;
	uint32_t vertex_element_size = 0;
	size_t vertext_buffer_offset = 0;
	// Indices
	uint32_t index_count = 0;
	uint32_t index_element_size = 0;
	size_t index_buffer_offset = 0;
};

class DAPI GeometrySystem {
public:
	static GeometrySystem& Get();

public:
	bool Initialize(IRenderer* renderer);
	void Shutdown();

	/*
	* @brief Acquires an existing geometry by id.
	* 
	* @param id The geometry identifier to acquire by.
	* @return A pointer to the acquired geometry or nullptr if failed.
	*/
	Geometry* AcquireByID(uint32_t id);

	/*
	* @brief Registers and acquires a new geometry using the given config.
	* 
	* @param config The geometry configuration.
	* @param auto_release Indicates if the acquired geometry should be unloaded when its reference count reaches 0.
	* @returna A pointer to the acquired geometry or nullptr if failed.
	*/
	Geometry* AcquireFromConfig(SGeometryConfig config, bool auto_release);

	/*
	* @brief Releases a reference to the provided geometry.
	* 
	* @prama geometry The geometry to be released.
	*/
	void Release(Geometry* geometry);

	/*
	*@brief Obtains a pointer to the default geometry.
	* 
	* @returns A pointer to the default geometry.
	*/
	Geometry* GetDefaultGeometry();

	/*
	*@brief Obtains a pointer to the default 2D geometry.
	*
	* @returns A pointer to the default 2D geometry.
	*/
	Geometry* GetDefaultGeometry2D();

	/*
	* @brief Generates configuration for plane geometries given the provided parameters.
	* NOTO: vertex and index arrays are dynamically allocated and should be feed upon object disposal.
	* Thus, this should not be considered production code.
	* 
	* @param width The overall width of the plane. Must be non-zero.
	* @param height The overall height of the plane. Must be non-zero.
	* @praam x_segment_count The number of segments alone the x-axis in the plane.
	* @param y_segment_count The number of segments alone the y-axis in the plane.
	* @param tile_x The number of times the texture should tile across the plane on the x-axis.
	* @param tile_y The number of times the texture should tile across the plane on the y-axis.
	* @param name The name of the generated geometry.
	* @param material_name The name of the material to be used.
	* @returns A geometry configuration which can then be fed into AcquireFromConfig().
	*/
	SGeometryConfig GeneratePlaneConfig(float width, float height, uint32_t x_segment_count, uint32_t y_segment_count,
		float tile_x, float tile_y, const FString& name, const FString& material_name);

	/*
	* @brief Generates configuration for cube geometries given the provided parameters.
	*
	* @param width The overall width of the plane. Must be non-zero.
	* @param height The overall height of the plane. Must be non-zero.
	* @param tile_x The number of times the texture should tile across the plane on the x-axis.
	* @param tile_y The number of times the texture should tile across the plane on the y-axis.
	* @param name The name of the generated geometry.
	* @param material_name The name of the material to be used.
	* @returns A geometry configuration which can then be fed into AcquireFromConfig().
	*/
	SGeometryConfig GenerateCubeConfig(float width, float height, 
		float depth, float tile_x, float tile_y, const FString& name, const FString& material_name);

	/*
	*@brief Obtains a pointer to the quad 2D geometry. Default generate full screen quad.
	*
	* @returns A pointer to the quad 2D geometry.
	*/
	Geometry* GenerateQuad(const FString& name, const FString& material_name);


	void ConfigDispose(SGeometryConfig* config);

private:
	bool CreateDefaultGeometries();
	Geometry* CreateGeometry(SGeometryConfig config);
	void DestroyGeometry(Geometry* geometry);

public:
	Geometry* DefaultGeometry = nullptr;
	Geometry* Default2DGeometry = nullptr;

	TArray<Geometry*> RegisteredGeometries;
	IRenderer* Renderer = nullptr;

	bool Initilized;
};