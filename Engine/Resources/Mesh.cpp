#include "Mesh.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/JobSystem.hpp"

void Mesh::LoadJobSuccess(void* params) {
	MeshLoadParams* MeshParams = (MeshLoadParams*)params;

	// This also handle the GPU upload. Can't be jobified until the renderer is multithread.
	SGeometryConfig* Configs = (SGeometryConfig*)MeshParams->mesh_resource.Data;
	MeshParams->out_mesh->geometry_count = (unsigned short)MeshParams->mesh_resource.DataCount;
	MeshParams->out_mesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * MeshParams->out_mesh->geometry_count, MemoryType::eMemory_Type_Array);
	for (uint32_t i = 0; i < MeshParams->out_mesh->geometry_count; ++i) {
		SGeometryConfig& Config = Configs[i];
		MeshParams->out_mesh->geometries[i] = GeometrySystem::AcquireFromConfig(Config, true);
	}
	MeshParams->out_mesh->Generation++;

	LOG_INFO("Successfully loaded mesh: '%s'.", MeshParams->resource_name.c_str());
	ResourceSystem::Unload(&MeshParams->mesh_resource);
}

void Mesh::LoadJobFail(void* params) {
	MeshLoadParams* MeshParams = (MeshLoadParams*)params;
	LOG_ERROR("Failed to load mesh: '%s'.", MeshParams->resource_name.c_str());
	ResourceSystem::Unload(&MeshParams->mesh_resource);
}

bool Mesh::LoadJobStart(void* params, void* result_data) {
	MeshLoadParams* LoadParams = (MeshLoadParams*)params;
	bool Result = ResourceSystem::Load(LoadParams->resource_name, ResourceType::eResource_type_Static_Mesh, nullptr, &LoadParams->mesh_resource);

	// NOTE: The load params are also used as the result data here, only the mesh)resource field is populated now.
	Memory::Copy(result_data, LoadParams, sizeof(MeshLoadParams));
	return Result;
}

bool Mesh::LoadFromResource(const std::string& resource_name) {
	Generation = INVALID_ID_U8;

	MeshLoadParams Params;
	Params.resource_name = resource_name;
	Params.out_mesh = this;
	Params.mesh_resource = {};
	Name = resource_name;

	JobInfo Job = JobSystem::CreateJob(
		std::bind(&Mesh::LoadJobStart, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&Mesh::LoadJobSuccess, this, std::placeholders::_1),
		std::bind(&Mesh::LoadJobFail, this, std::placeholders::_1),
		std::make_shared<MeshLoadParams>(Params), 
		sizeof(MeshLoadParams), 
		sizeof(MeshLoadParams));
	JobSystem::Submit(Job);

	return true;
}

void Mesh::Unload() {
	for (uint32_t i = 0; i < geometry_count; ++i) {
		GeometrySystem::Release(geometries[i]);
	}

	Memory::Free(geometries, sizeof(Geometry*) * geometry_count, MemoryType::eMemory_Type_Array);
	geometries = nullptr;

	// For good measure. Invalidate the geometry so it doesn't attemp to be renderer.
	geometry_count = 0;
	Generation = INVALID_ID_U8;
}
