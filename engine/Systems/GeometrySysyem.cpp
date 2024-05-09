#include "GeometrySystem.h"

#include "Renderer/RendererFrontend.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

SGeometrySystemConfig GeometrySystem::GeometrySystemConfig;
Geometry GeometrySystem::DefaultGeometry;
Geometry GeometrySystem::Default2DGeometry;
SGeometryReference* GeometrySystem::RegisteredGeometries = nullptr;
bool GeometrySystem::Initilized = false;
IRenderer* GeometrySystem::Renderer = nullptr;

bool GeometrySystem::Initialize(IRenderer* renderer, SGeometrySystemConfig config) {
	if (config.max_geometry_count == 0) {
		UL_FATAL("Geometry system initialize failed. config.max_geometry_count must be > 0.");
		return false;
	}

	if (renderer == nullptr) {
		UL_FATAL("Material system init failed. Renderer is nullptr.");
		return false;
	}

	GeometrySystemConfig = config;
	Renderer = renderer;

	// Invalidate all geometries in the array.
	RegisteredGeometries = (SGeometryReference*)Memory::Allocate(sizeof(SGeometryReference) * config.max_geometry_count, MemoryType::eMemory_Type_Array);
	for (uint32_t i = 0; i < config.max_geometry_count; ++i) {
		RegisteredGeometries[i].geometry.ID = INVALID_ID;
		RegisteredGeometries[i].geometry.InternalID= INVALID_ID;
		RegisteredGeometries[i].geometry.Generation = INVALID_ID;
	}

	if (!CreateDefaultGeometries()) {
		UL_FATAL("Failed to create default geometries. Application quit now!");
		return false;
	}

	Initilized = true;
	return true;
}

void GeometrySystem::Shutdown() {
	Initilized = false;
}

Geometry* GeometrySystem::AcquireByID(uint32_t id) {
	if (id != INVALID_ID && RegisteredGeometries[id].geometry.ID != INVALID_ID) {
		RegisteredGeometries[id].reference_count++;
		return &RegisteredGeometries[id].geometry;
	}

	// NOTE: Should return default geometry instead.
	UL_ERROR("Geometry system acquire by id cannot load invalid id. Reutrn nullptr.");
	return nullptr;
}

Geometry* GeometrySystem::AcquireFromConfig(SGeometryConfig config, bool auto_release) {
	Geometry* geometry = nullptr;
	for (uint32_t i = 0; i < GeometrySystemConfig.max_geometry_count; ++i) {
		if (RegisteredGeometries[i].geometry.ID == INVALID_ID) {
			// Found empty slot.
			RegisteredGeometries[i].auto_release = auto_release;
			RegisteredGeometries[i].reference_count = 1;
			geometry = &RegisteredGeometries[i].geometry;
			geometry->ID = i;
			break;
		}
	}

	if (geometry == nullptr) {
		UL_ERROR("Unable to obtain free slot for geometry. Adjust configuration to allow more space. Returning nullptr.");
		return nullptr;
	}

	if (!CreateGeometry(config, geometry)) {
		UL_ERROR("Failed to create geometry. Returning nullptr.");
		return nullptr;
	}

	return geometry;
}

void GeometrySystem::Release(Geometry* geometry) {
	if (geometry == nullptr) {
		return;
	}

	if (geometry->ID != INVALID_ID) {
		SGeometryReference* Ref = &RegisteredGeometries[geometry->ID];

		// Take a copy of id.
		uint32_t id = geometry->ID;
		if (Ref->geometry.ID == geometry->ID) {
			if (Ref->reference_count > 0) {
				Ref->reference_count--;
			}

			// Also blanks out the geometry id.
			if (Ref->reference_count < 1 && Ref->auto_release) {
				DestroyGeometry(geometry);
				Ref->reference_count = 0;
				Ref->auto_release = false;
			}
		}
		else {
			UL_FATAL("Geometry id mismatch. Check registration logic. as this should never occur.");
		}

		return;
	}

	UL_WARN("Geometry system release by id can not release invalid geometry id. Nothing was down.");
}

Geometry* GeometrySystem::GetDefaultGeometry() {
	if (Initilized) {
		return &DefaultGeometry;
	}

	UL_FATAL("Get default Geometry called before system initialize. Returning nullptr");
	return nullptr;
}

Geometry* GeometrySystem::GetDefaultGeometry2D() {
	if (Initilized) {
		return &Default2DGeometry;
	}

	UL_FATAL("Get default 2D Geometry called before system initialize. Returning nullptr");
	return nullptr;
}

bool GeometrySystem::CreateDefaultGeometries() {
	const float f = 5.0f;
	Vertex Verts[4];
	Memory::Zero(Verts, sizeof(Vertex) * 4);

	Verts[0].position.x = -0.5f * f;
	Verts[0].position.y = -0.5f * f;
	Verts[0].texcoord.x = 0.0f;
	Verts[0].texcoord.y = 0.0f;

	Verts[1].position.x = 0.5f * f;
	Verts[1].position.y = 0.5f * f;
	Verts[1].texcoord.x = 1.0f;
	Verts[1].texcoord.y = 0.0f;

	Verts[2].position.x = -0.5f * f;
	Verts[2].position.y = 0.5f * f;
	Verts[2].texcoord.x = 0.0f;
	Verts[2].texcoord.y = 1.0f;

	Verts[3].position.x = 0.5f * f;
	Verts[3].position.y = -0.5f * f;
	Verts[3].texcoord.x = 1.0f;
	Verts[3].texcoord.y = 1.0f;

	uint32_t Indices[6] = { 0, 1, 2, 0, 3, 1 };

	// Send the geometry off to the renderer to be uploaded to the GPU.
	if (!Renderer->CreateGeometry(&DefaultGeometry, sizeof(Vertex), 4, Verts, sizeof(uint32_t), 6, Indices)) {
		UL_FATAL("Failed to create default geometry. Application quit now!");
		return false;
	}

	// Acquire the default material.
	DefaultGeometry.Material = MaterialSystem::GetDefaultMaterial();

	// Create default 2d geometry.
	const float uf = 100.0f;
	Vertex2D Verts2D[4];
	Memory::Zero(Verts2D, sizeof(Vertex2D) * 4);
	Verts2D[0].position.x = -0.5f * uf;	// 0    3
	Verts2D[0].position.y = -0.5f * uf;	//
	Verts2D[0].texcoord.x = 0.0f;		//
	Verts2D[0].texcoord.y = 0.0f;		// 2    1

	Verts2D[1].position.x = 0.5f * uf;
	Verts2D[1].position.y = 0.5f * uf;
	Verts2D[1].texcoord.x = 1.0f;
	Verts2D[1].texcoord.y = 1.0f;

	Verts2D[2].position.x = -0.5f * uf;
	Verts2D[2].position.y = 0.5f * uf;
	Verts2D[2].texcoord.x = 0.0f;
	Verts2D[2].texcoord.y = 1.0f;

	Verts2D[3].position.x = 0.5f * uf;
	Verts2D[3].position.y = -0.5f * uf;
	Verts2D[3].texcoord.x = 1.0f;
	Verts2D[3].texcoord.y = 0.0f;

	// Indices NOTO: counter-clockwise.
	uint32_t Indices2D[6] = { 2, 1, 0, 3, 0, 1 };

	// Send the geometry off to the renderer to be uploaded to the GPU.
	if (!Renderer->CreateGeometry(&Default2DGeometry, sizeof(Vertex2D), 4, Verts2D, sizeof(uint32_t), 6, Indices2D)) {
		UL_FATAL("Failed to create default 2d geometry. Application quit now!");
		return false;
	}

	// Acquire the default material.
	Default2DGeometry.Material = MaterialSystem::GetDefaultMaterial();

	return true;
}

bool GeometrySystem::CreateGeometry(SGeometryConfig config, Geometry* geometry) {
	// Send the geometry off to the renderer to be uploaded to the GPU.
	if (!Renderer->CreateGeometry(geometry, config.vertex_size, config.vertex_count, config.vertices, config.index_size, config.index_count, config.indices)) {
		// Invalidate the entry.
		RegisteredGeometries[geometry->ID].reference_count = 0;
		RegisteredGeometries[geometry->ID].auto_release = false;

		geometry->ID = INVALID_ID;
		geometry->Generation = INVALID_ID;
		geometry->InternalID = INVALID_ID;

		return false;
	}

	// Acquire the material.
	if (strlen(config.name) > 0) {
		geometry->Material = MaterialSystem::Acquire(config.material_name);
		if (geometry->Material == nullptr) {
			geometry->Material = MaterialSystem::GetDefaultMaterial();
		}
	}

	return true;
}

void GeometrySystem::DestroyGeometry(Geometry* geometry) {
	Renderer->DestroyGeometry(geometry);
	geometry->ID = INVALID_ID;
	geometry->Generation = INVALID_ID;
	geometry->InternalID = INVALID_ID;

	geometry->name[0] = '0';

	// Release the material.
	if (geometry->Material && strlen(geometry->name) > 0) {
		MaterialSystem::Release(geometry->Material->Name);
		geometry->Material = nullptr;
	}
}

SGeometryConfig GeometrySystem::GeneratePlaneConfig(float width, float height, uint32_t x_segment_count,
	uint32_t y_segment_count, float tile_x, float tile_y, const char* name, const char* material_name) {
	if (width == 0) {
		UL_WARN("width must be non-zero. Defauting to one.");
		width = 1.0f;
	}

	if (height == 0) {
		UL_WARN("height must be non-zero. Defauting to one.");
		height = 1.0f;
	}

	if (x_segment_count < 1) {
		UL_WARN("x_segment_count must be non-zero. Defauting to one.");
		x_segment_count = 1;
	}

	if (y_segment_count < 1) {
		UL_WARN("y_segment_count must be non-zero. Defauting to one.");
		y_segment_count = 1;
	}

	if (tile_x == 0) {
		UL_WARN("width must be non-zero. Defauting to one.");
		tile_x = 1.0f;
	}

	if (tile_y == 0) {
		UL_WARN("width must be non-zero. Defauting to one.");
		tile_y = 1.0f;
	}

	SGeometryConfig Config;
	Config.vertex_size = sizeof(Vertex);
	Config.vertex_count = x_segment_count * y_segment_count * 4; // 4 vertex per segment.
	Config.vertices = (Vertex*)Memory::Allocate(sizeof(Vertex) * Config.vertex_count, MemoryType::eMemory_Type_Array);
	Config.index_size = sizeof(uint32_t);
	Config.index_count = x_segment_count * y_segment_count * 6; // 6 index per segment.
	Config.indices = (uint32_t*)Memory::Allocate(sizeof(uint32_t) * Config.index_count, MemoryType::eMemory_Type_Array);

	// TODO: This generates extra vertices, but we can always de-duplicate them later.
	float seg_width = width / x_segment_count;
	float seg_height = height / y_segment_count;
	float half_width = width * 0.5f;
	float half_height = height * 0.5f;
	for (uint32_t y = 0; y < y_segment_count; ++y) {
		for (uint32_t x = 0; x < x_segment_count; ++x) {
			// Generate vertices.
			float min_x = (x * seg_width) - half_width;
			float min_y = (y * seg_height) - half_height;
			float max_x = min_x + seg_width;
			float max_y = min_y + seg_height;
			float min_uvx = (x / (float)x_segment_count) * tile_x;
			float min_uvy = (y / (float)y_segment_count) * tile_y;
			float max_uvx = ((x + 1) / (float)x_segment_count) * tile_x;
			float max_uvy = ((y + 1) / (float)y_segment_count) * tile_y;

			uint32_t v_offset = ((y * x_segment_count) + x) * 4;
			Vertex* v0 = &((Vertex*)Config.vertices)[v_offset + 0];
			Vertex* v1 = &((Vertex*)Config.vertices)[v_offset + 1];
			Vertex* v2 = &((Vertex*)Config.vertices)[v_offset + 2];
			Vertex* v3 = &((Vertex*)Config.vertices)[v_offset + 3];

			v0->position.x = min_x;
			v0->position.y = min_y;
			v0->texcoord.x = min_uvx;
			v0->texcoord.y = min_uvy;

			v1->position.x = max_x;
			v1->position.y = max_y;
			v1->texcoord.x = max_uvx;
			v1->texcoord.y = max_uvy;

			v2->position.x = min_x;
			v2->position.y = max_y;
			v2->texcoord.x = min_uvx;
			v2->texcoord.y = max_uvy;

			v3->position.x = max_x;
			v3->position.y = min_y;
			v3->texcoord.x = max_uvx;
			v3->texcoord.y = min_uvy;

			// Generate indices.
			uint32_t i_offset = ((y * x_segment_count) + x) * 6;
			((uint32_t*)Config.indices)[i_offset + 0] = v_offset + 0;
			((uint32_t*)Config.indices)[i_offset + 1] = v_offset + 1;
			((uint32_t*)Config.indices)[i_offset + 2] = v_offset + 2;
			((uint32_t*)Config.indices)[i_offset + 3] = v_offset + 0;
			((uint32_t*)Config.indices)[i_offset + 4] = v_offset + 3;
			((uint32_t*)Config.indices)[i_offset + 5] = v_offset + 1;
		}
	}

	if (name && strlen(name) > 0) {
		strncpy(Config.name, name, GEOMETRY_NAME_MAX_LENGTH);
	}
	else {
		strncpy(Config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
	}

	if (material_name && material_name > 0) {
		strncpy(Config.material_name, material_name, MATERIAL_NAME_MAX_LENGTH);
	}
	else {
		strncpy(Config.material_name, DEFAULT_MATERIAL_NAME, MATERIAL_NAME_MAX_LENGTH);
	}

	return Config;
}
