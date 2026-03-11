#include "StaticMeshActor.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/JobSystem.hpp"
#include "Rendering/RenderTypes.hpp"

void AStaticMeshActor::Draw() {
	for (uint32_t j = 0; j < geometry_count; j++) {
		GeometryRenderData RenderData;
		RenderData.geometry = geometries[j];
		RenderData.model_mat = GetWorldTransform();
		RenderData.uniqueID = GetUniqueID();
	}
}

void AStaticMeshActor::LoadJobSuccess(void* params) {
	FMeshLoadParams* MeshParams = (FMeshLoadParams*)params;

	// This also handle the GPU upload. Can't be jobified until the renderer is multithread.
	SGeometryConfig* Configs = (SGeometryConfig*)MeshParams->mesh_resource.Data;
	MeshParams->out_mesh->geometry_count = (unsigned short)MeshParams->mesh_resource.DataCount;
	MeshParams->out_mesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * MeshParams->out_mesh->geometry_count, MemoryType::eMemory_Type_Array);
	for (uint32_t i = 0; i < MeshParams->out_mesh->geometry_count; ++i) {
		SGeometryConfig& Config = Configs[i];
		MeshParams->out_mesh->geometries[i] = GeometrySystem::AcquireFromConfig(Config, true);
	}
	MeshParams->out_mesh->Generation++;

	GLOG(Log::eInfo, "Successfully loaded mesh: '%s'.", MeshParams->resource_name.CStr());
	ResourceSystem::Unload(&MeshParams->mesh_resource);
}

void AStaticMeshActor::LoadJobFail(void* params) {
	FMeshLoadParams* MeshParams = (FMeshLoadParams*)params;
	GLOG(Log::eError, "Failed to load mesh: '%s'.", MeshParams->resource_name.CStr());
	ResourceSystem::Unload(&MeshParams->mesh_resource);
}

bool AStaticMeshActor::LoadJobStart(void* params, void* result_data) {
	FMeshLoadParams* LoadParams = (FMeshLoadParams*)params;
	bool Result = ResourceSystem::Load(LoadParams->resource_name, EAssetType::StaticMesh, nullptr, &LoadParams->mesh_resource);

	// NOTE: The load params are also used as the result data here, only the mesh)resource field is populated now.
	Memory::Copy(result_data, LoadParams, sizeof(FMeshLoadParams));
	return Result;
}

bool AStaticMeshActor::LoadFromResource(const FString& resource_name) {
	Generation = INVALID_ID_U8;

	FMeshLoadParams Params;
	Params.resource_name = resource_name;
	Params.out_mesh = this;
	Params.mesh_resource = {};
	Name_ = resource_name;

	JobInfo Job = JobSystem::CreateJob(
		std::bind(&AStaticMeshActor::LoadJobStart, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&AStaticMeshActor::LoadJobSuccess, this, std::placeholders::_1),
		std::bind(&AStaticMeshActor::LoadJobFail, this, std::placeholders::_1),
		std::make_shared<FMeshLoadParams>(Params), 
		sizeof(FMeshLoadParams), 
		sizeof(FMeshLoadParams));
	JobSystem::Submit(Job);

	return true;
}

void AStaticMeshActor::Unload() {
	for (uint32_t i = 0; i < geometry_count; ++i) {
		GeometrySystem::Release(geometries[i]);
	}

	Memory::Free(geometries, MemoryType::eMemory_Type_Array);
	geometries = nullptr;

	// For good measure. Invalidate the geometry so it doesn't attemp to be renderer.
	geometry_count = 0;
	Generation = INVALID_ID_U8;
}
