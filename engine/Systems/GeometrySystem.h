#pragma once

#include "Math/MathTypes.hpp"
#include "MaterialSystem.h"
#include "Resources/Geometry.hpp"

#define GEOMETRY_MAX_COUNT 4096
#define DEFAULT_GEOMETRY_NAME "default"

struct GeometryData {
	uint32_t id = INVALID_ID;
	uint32_t generation = INVALID_ID;
	// Vertices
	uint32_t vertex_count;
	uint32_t vertex_element_size;
	size_t vertext_buffer_offset;
	// Indices
	uint32_t index_count;
	uint32_t index_element_size;
	size_t index_buffer_offset;
};

struct SGeometryConfig {
	// Vertices
	uint32_t vertex_size;
	uint32_t vertex_count;
	void* vertices;

	// Indices
	uint32_t index_size;
	uint32_t index_count;
	void* indices;

	char name[GEOMETRY_NAME_MAX_LENGTH];
	char material_name[MATERIAL_NAME_MAX_LENGTH];
};

struct SGeometryReference {
	size_t reference_count;
	Geometry geometry;
	bool auto_release;
};

struct SGeometrySystemConfig {
	// Max number of geometries that can be loaded at once.
	// NOTO: Should be significantly greater than the numver of static meshes because
	// the there can and will be more than one of ehese per mesh.
	// Take other system into account as will.
	uint32_t max_geometry_count;
};

class GeometrySystem {
public:
	static bool Initialize(IRenderer* renderer, SGeometrySystemConfig config);
	static void Shutdown();

	/*
	* @brief Acquires an existing geometry by id.
	* 
	* @param id The geometry identifier to acquire by.
	* @return A pointer to the acquired geometry or nullptr if failed.
	*/
	static Geometry* AcquireByID(uint32_t id);

	/*
	* @brief Registers and acquires a new geometry using the given config.
	* 
	* @param config The geometry configuration.
	* @param auto_release Indicates if the acquired geometry should be unloaded when its reference count reaches 0.
	* @returna A pointer to the acquired geometry or nullptr if failed.
	*/
	static Geometry* AcquireFromConfig(SGeometryConfig config, bool auto_release);

	/*
	* @brief Releases a reference to the provided geometry.
	* 
	* @prama geometry The geometry to be released.
	*/
	static void Release(Geometry* geometry);

	/*
	*@brief Obtains a pointer to the default geometry.
	* 
	* @returns A pointer to the default geometry.
	*/
	static Geometry* GetDefaultGeometry();

	/*
	*@brief Obtains a pointer to the default 2D geometry.
	*
	* @returns A pointer to the default 2D geometry.
	*/
	static Geometry* GetDefaultGeometry2D();

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
	static SGeometryConfig GeneratePlaneConfig(float width, float height, uint32_t x_segment_count, uint32_t y_segment_count,
		float tile_x, float tile_y, const char* name, const char* material_name);

private:
	static bool CreateDefaultGeometries();
	static bool CreateGeometry(SGeometryConfig config, Geometry* geometry);
	static void DestroyGeometry(Geometry* geometry);

public:
	static SGeometrySystemConfig GeometrySystemConfig;

	static Geometry DefaultGeometry;
	static Geometry Default2DGeometry;

	static SGeometryReference* RegisteredGeometries;
	static IRenderer* Renderer;

	static bool Initilized;
};