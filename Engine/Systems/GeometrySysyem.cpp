#include "GeometrySystem.h"

#include "Rendering/RendererFrontend.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Math/GeometryUtils.hpp"

SGeometrySystemConfig GeometrySystem::GeometrySystemConfig;
Geometry GeometrySystem::DefaultGeometry;
Geometry GeometrySystem::Default2DGeometry;
SGeometryReference* GeometrySystem::RegisteredGeometries = nullptr;
bool GeometrySystem::Initilized = false;
IRenderer* GeometrySystem::Renderer = nullptr;

bool GeometrySystem::Initialize(IRenderer* renderer, SGeometrySystemConfig config) {
	if (config.max_geometry_count == 0) {
		GLOG(Log::eFatal, "Geometry system initialize failed. config.max_geometry_count must be > 0.");
		return false;
	}

	if (renderer == nullptr) {
		GLOG(Log::eFatal, "Material system init failed. Renderer is nullptr.");
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
		GLOG(Log::eFatal, "Failed to create default geometries. Application quit now!");
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
	GLOG(Log::eError, "Geometry system acquire by id cannot load invalid id. Reutrn nullptr.");
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
		GLOG(Log::eError, "Unable to obtain free slot for geometry. Adjust configuration to allow more space. Returning nullptr.");
		return nullptr;
	}

	if (!CreateGeometry(config, geometry)) {
		GLOG(Log::eError, "Failed to create geometry '%s'. Returning nullptr.", config.name.c_str());
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
			GLOG(Log::eFatal, "Geometry id mismatch. Check registration logic. as this should never occur.");
		}

		return;
	}

	GLOG(Log::eWarn, "Geometry system release by id can not release invalid geometry id. Nothing was down.");
}

Geometry* GeometrySystem::GetDefaultGeometry() {
	if (Initilized) {
		return &DefaultGeometry;
	}

	GLOG(Log::eFatal, "Get default Geometry called before system initialize. Returning nullptr");
	return nullptr;
}

Geometry* GeometrySystem::GetDefaultGeometry2D() {
	if (Initilized) {
		return &Default2DGeometry;
	}

	GLOG(Log::eFatal, "Get default 2D Geometry called before system initialize. Returning nullptr");
	return nullptr;
}

bool GeometrySystem::CreateDefaultGeometries() {
	const float f = 5.0f;
	Vertex Verts[4];
	Memory::Zero(Verts, sizeof(Vertex) * 4);

	Verts[0].position.x = -0.5f * f;
	Verts[0].position.y = 0.5f * f;
	Verts[0].texcoord.x = 0.0f;
	Verts[0].texcoord.y = 0.0f;

	Verts[1].position.x = 0.5f * f;
	Verts[1].position.y = -0.5f * f;
	Verts[1].texcoord.x = 1.0f;
	Verts[1].texcoord.y = 0.0f;

	Verts[2].position.x = -0.5f * f;
	Verts[2].position.y = -0.5f * f;
	Verts[2].texcoord.x = 0.0f;
	Verts[2].texcoord.y = 1.0f;

	Verts[3].position.x = 0.5f * f;
	Verts[3].position.y = 0.5f * f;
	Verts[3].texcoord.x = 1.0f;
	Verts[3].texcoord.y = 1.0f;

	uint32_t Indices[6] = { 0, 1, 2, 0, 3, 1 };

	// Send the geometry off to the renderer to be uploaded to the GPU.
	DefaultGeometry.InternalID = INVALID_ID;
	if (!Renderer->CreateGeometry(&DefaultGeometry, sizeof(Vertex), 4, Verts, sizeof(uint32_t), 6, Indices)) {
		GLOG(Log::eFatal, "Failed to create default geometry. Application quit now!");
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
	Default2DGeometry.InternalID = INVALID_ID;
	if (!Renderer->CreateGeometry(&Default2DGeometry, sizeof(Vertex2D), 4, Verts2D, sizeof(uint32_t), 6, Indices2D)) {
		GLOG(Log::eFatal, "Failed to create default 2d geometry. Application quit now!");
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

	// Copy over extents, center. etc.
	geometry->Center = config.center;
	geometry->Extents.min = config.min_extents;
	geometry->Extents.max = config.max_extents;
	geometry->name = std::move(config.name);

	// Acquire the material.
	if (config.material_name.length() > 0) {
		geometry->Material = MaterialSystem::Acquire(config.material_name.c_str());
	}

	if (geometry->Material == nullptr) {
		GLOG(Log::eWarn, "Default use default material.");
		geometry->Material = MaterialSystem::GetDefaultMaterial();
	}

	return true;
}

void GeometrySystem::ConfigDispose(SGeometryConfig* config) {
	if (config) {
		if (config->vertices) {
			Memory::Free(config->vertices, MemoryType::eMemory_Type_Array);
		}
		if (config->indices) {
			Memory::Free(config->indices, MemoryType::eMemory_Type_Array);
		}
		Memory::Zero(config, sizeof(SGeometryConfig));
	}
}

void GeometrySystem::DestroyGeometry(Geometry* geometry) {
	Renderer->DestroyGeometry(geometry);
	geometry->ID = INVALID_ID;
	geometry->Generation = INVALID_ID;
	geometry->InternalID = INVALID_ID;

	geometry->name[0] = '0';

	// Release the material.
	if (geometry->Material && geometry->Material->Name.length() > 0) {
		MaterialSystem::Release(geometry->Material->Name.c_str());
		geometry->Material = nullptr;
	}
}

SGeometryConfig GeometrySystem::GeneratePlaneConfig(float width, float height, uint32_t x_segment_count,
	uint32_t y_segment_count, float tile_x, float tile_y, const std::string& name, const std::string& material_name) {
	if (width == 0) {
		GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
		width = 1.0f;
	}

	if (height == 0) {
		GLOG(Log::eWarn, "height must be non-zero. Defauting to one.");
		height = 1.0f;
	}

	if (x_segment_count < 1) {
		GLOG(Log::eWarn, "x_segment_count must be non-zero. Defauting to one.");
		x_segment_count = 1;
	}

	if (y_segment_count < 1) {
		GLOG(Log::eWarn, "y_segment_count must be non-zero. Defauting to one.");
		y_segment_count = 1;
	}

	if (tile_x == 0) {
		GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
		tile_x = 1.0f;
	}

	if (tile_y == 0) {
		GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
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
			v0->normal = Vector3(0.0f, 1.0f, 0.0f);
			v0->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
			v0->tangent = Vector4(0.0, 0.0f, 0.0, 1.0f);

			v1->position.x = max_x;
			v1->position.y = max_y;
			v1->texcoord.x = max_uvx;
			v1->texcoord.y = max_uvy;
			v1->normal = Vector3(0.0f, 1.0f, 0.0f);
			v1->color = Vector3(0.0f, 1.0f, 0.0f);
			v1->tangent = Vector4(0.0, 0.0f, 0.0, 1.0f);

			v2->position.x = min_x;
			v2->position.y = max_y;
			v2->texcoord.x = min_uvx;
			v2->texcoord.y = max_uvy;
			v2->normal = Vector3(0.0f, 1.0f, 0.0f);
			v2->color = Vector3(0.0f, 1.0f, 0.0f);
			v2->tangent = Vector4(0.0, 0.0f, 0.0, 1.0f);

			v3->position.x = max_x;
			v3->position.y = min_y;
			v3->texcoord.x = max_uvx;
			v3->texcoord.y = min_uvy;
			v3->normal = Vector3(0.0f, 1.0f, 0.0f);
			v3->color = Vector3(0.0f, 1.0f, 0.0f);
			v3->tangent = Vector4(0.0, 0.0f, 0.0, 1.0f);

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

	Config.name = name.empty() ? DEFAULT_GEOMETRY_PLANE_NAME : name;
	Config.material_name = material_name.empty() ? DEFAULT_MATERIAL_NAME : material_name;

	return Config;
}

SGeometryConfig GeometrySystem::GenerateCubeConfig(float width, float height,
	float depth, float tile_x, float tile_y, const std::string& name, const std::string& material_name) {
	if (width == 0) {
			GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
			width = 1.0f;
		}

		if (height == 0) {
			GLOG(Log::eWarn, "height must be non-zero. Defauting to one.");
			height = 1.0f;
		}

		if (depth == 0) {
			GLOG(Log::eWarn, "height must be non-zero. Defauting to one.");
			height = 1.0f;
		}

		if (tile_x == 0) {
			GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
			tile_x = 1.0f;
		}

		if (tile_y == 0) {
			GLOG(Log::eWarn, "width must be non-zero. Defauting to one.");
			tile_y = 1.0f;
		}

		SGeometryConfig Config;
		Config.vertex_size = sizeof(Vertex);
		Config.vertex_count = 6 * 4; // 4 vertex per segment.
		Config.vertices = (Vertex*)Memory::Allocate(sizeof(Vertex) * Config.vertex_count, MemoryType::eMemory_Type_Array);
		Config.index_size = sizeof(uint32_t);
		Config.index_count = 6 * 6; // 6 index per segment.
		Config.indices = (uint32_t*)Memory::Allocate(sizeof(uint32_t) * Config.index_count, MemoryType::eMemory_Type_Array);

		float HalfWidth = width * 0.5f;
		float HalfHeight = height * 0.5f;
		float HalfDepth = depth * 0.5f;
		float MinX = -HalfWidth;
		float MinY = -HalfHeight;
		float MinZ = -HalfDepth;
		float MaxX = HalfWidth;
		float MaxY = HalfHeight;
		float MaxZ = HalfDepth;
		float MinUVX = 0.0f;
		float MinUVY = 0.0f;
		float MaxUVX = tile_x;
		float MaxUVY = tile_y;

		Config.min_extents = { MinX, MinY, MinZ };
		Config.max_extents = { MaxX, MaxY, MaxZ };
		// Always 0 since min/max of each axis are -/+ half of the size.
		Config.center = { 0, 0, 0 };

		Vertex Verts[24];

		// Front face
		Verts[(0 * 4) + 0].position = Vector3(MinX, MinY, MaxZ);
		Verts[(0 * 4) + 1].position = Vector3(MaxX, MaxY, MaxZ);
		Verts[(0 * 4) + 2].position = Vector3(MinX, MaxY, MaxZ);
		Verts[(0 * 4) + 3].position = Vector3(MaxX, MinY, MaxZ);
		Verts[(0 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(0 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(0 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(0 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(0 * 4) + 0].normal = Vector3(0.0f, 0.0f, 1.0f);
		Verts[(0 * 4) + 1].normal = Vector3(0.0f, 0.0f, 1.0f);
		Verts[(0 * 4) + 2].normal = Vector3(0.0f, 0.0f, 1.0f);
		Verts[(0 * 4) + 3].normal = Vector3(0.0f, 0.0f, 1.0f);

		// Back face
		Verts[(1 * 4) + 0].position = Vector3(MaxX, MinY, MinZ);
		Verts[(1 * 4) + 1].position = Vector3(MinX, MaxY, MinZ);
		Verts[(1 * 4) + 2].position = Vector3(MaxX, MaxY, MinZ);
		Verts[(1 * 4) + 3].position = Vector3(MinX, MinY, MinZ);
		Verts[(1 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(1 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(1 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(1 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(1 * 4) + 0].normal = Vector3(0.0f, 0.0f, -1.0f);
		Verts[(1 * 4) + 1].normal = Vector3(0.0f, 0.0f, -1.0f);
		Verts[(1 * 4) + 2].normal = Vector3(0.0f, 0.0f, -1.0f);
		Verts[(1 * 4) + 3].normal = Vector3(0.0f, 0.0f, -1.0f);

		// Left face
		Verts[(2 * 4) + 0].position = Vector3(MinX, MinY, MinZ);
		Verts[(2 * 4) + 1].position = Vector3(MinX, MaxY, MaxZ);
		Verts[(2 * 4) + 2].position = Vector3(MinX, MaxY, MinZ);
		Verts[(2 * 4) + 3].position = Vector3(MinX, MinY, MaxZ);
		Verts[(2 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(2 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(2 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(2 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(2 * 4) + 0].normal = Vector3(-1.0f, 0.0f, 0.0f);
		Verts[(2 * 4) + 1].normal = Vector3(-1.0f, 0.0f, 0.0f);
		Verts[(2 * 4) + 2].normal = Vector3(-1.0f, 0.0f, 0.0f);
		Verts[(2 * 4) + 3].normal = Vector3(-1.0f, 0.0f, 0.0f);

		// Right face
		Verts[(3 * 4) + 0].position = Vector3(MaxX, MinY, MaxZ);
		Verts[(3 * 4) + 1].position = Vector3(MaxX, MaxY, MinZ);
		Verts[(3 * 4) + 2].position = Vector3(MaxX, MaxY, MaxZ);
		Verts[(3 * 4) + 3].position = Vector3(MaxX, MinY, MinZ);
		Verts[(3 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(3 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(3 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(3 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(3 * 4) + 0].normal = Vector3(1.0f, 0.0f, 0.0f);
		Verts[(3 * 4) + 1].normal = Vector3(1.0f, 0.0f, 0.0f);
		Verts[(3 * 4) + 2].normal = Vector3(1.0f, 0.0f, 0.0f);
		Verts[(3 * 4) + 3].normal = Vector3(1.0f, 0.0f, 0.0f);

		// Bottom face
		Verts[(4 * 4) + 0].position = Vector3(MaxX, MinY, MaxZ);
		Verts[(4 * 4) + 1].position = Vector3(MinX, MinY, MinZ);
		Verts[(4 * 4) + 2].position = Vector3(MaxX, MinY, MinZ);
		Verts[(4 * 4) + 3].position = Vector3(MinX, MinY, MaxZ);
		Verts[(4 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(4 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(4 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(4 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(4 * 4) + 0].normal = Vector3(0.0f, -1.0f, 0.0f);
		Verts[(4 * 4) + 1].normal = Vector3(0.0f, -1.0f, 0.0f);
		Verts[(4 * 4) + 2].normal = Vector3(0.0f, -1.0f, 0.0f);
		Verts[(4 * 4) + 3].normal = Vector3(0.0f, -1.0f, 0.0f);

		// Top face
		Verts[(5 * 4) + 0].position = Vector3(MinX, MaxY, MaxZ);
		Verts[(5 * 4) + 1].position = Vector3(MaxX, MaxY, MinZ);
		Verts[(5 * 4) + 2].position = Vector3(MinX, MaxY, MinZ);
		Verts[(5 * 4) + 3].position = Vector3(MaxX, MaxY, MaxZ);
		Verts[(5 * 4) + 0].texcoord = Vector2f(MinUVX, MinUVY);
		Verts[(5 * 4) + 1].texcoord = Vector2f(MaxUVX, MaxUVY);
		Verts[(5 * 4) + 2].texcoord = Vector2f(MinUVX, MaxUVY);
		Verts[(5 * 4) + 3].texcoord = Vector2f(MaxUVX, MinUVY);
		Verts[(5 * 4) + 0].normal = Vector3(0.0f, 1.0f, 0.0f);
		Verts[(5 * 4) + 1].normal = Vector3(0.0f, 1.0f, 0.0f);
		Verts[(5 * 4) + 2].normal = Vector3(0.0f, 1.0f, 0.0f);
		Verts[(5 * 4) + 3].normal = Vector3(0.0f, 1.0f, 0.0f);

		Memory::Copy(Config.vertices, Verts, Config.vertex_size* Config.vertex_count);

		for (uint32_t i = 0; i < 6; ++i) {
			uint32_t OffsetV = i * 4;
			uint32_t OffsetI = i * 6;
			((uint32_t*)Config.indices)[OffsetI + 0] = OffsetV + 0;
			((uint32_t*)Config.indices)[OffsetI + 1] = OffsetV + 1;
			((uint32_t*)Config.indices)[OffsetI + 2] = OffsetV + 2;
			((uint32_t*)Config.indices)[OffsetI + 3] = OffsetV + 0;
			((uint32_t*)Config.indices)[OffsetI + 4] = OffsetV + 3;
			((uint32_t*)Config.indices)[OffsetI + 5] = OffsetV + 1;
		}

		Config.name = name.empty() ? DEFAULT_GEOMETRY_PLANE_NAME : name;
		Config.material_name = material_name.empty() ? DEFAULT_MATERIAL_NAME : material_name;

		GeometryUtils::GenerateTangents(Config.vertex_count, (Vertex*)Config.vertices, Config.index_count, (uint32_t*)Config.indices);

		return Config;
}

Geometry* GeometrySystem::GenerateQuad(const std::string& name, const std::string& material_name) {
	SGeometryConfig Config = GeneratePlaneConfig(2, 2, 1, 1, 1, 1, name, material_name);
	Geometry* NewGeom = AcquireFromConfig(Config, true);
	if (!NewGeom) {
		GLOG(Log::eWarn, "Generated simple fullscreen quad geometry configuration falied.");
		return nullptr;
	}

	return NewGeom;
}