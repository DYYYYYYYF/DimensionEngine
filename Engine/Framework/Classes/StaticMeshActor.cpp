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

void AStaticMeshActor::LoadJobSuccess() {
	// This also handle the GPU upload. Can't be jobified until the renderer is multithread.
	SGeometryConfig* Configs = (SGeometryConfig*)LoadParams.mesh_resource.Data;
	LoadParams.out_mesh->geometry_count = (unsigned short)LoadParams.mesh_resource.DataCount;
	LoadParams.out_mesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * LoadParams.out_mesh->geometry_count, MemoryType::eMemory_Type_Array);
	for (uint32_t i = 0; i < LoadParams.out_mesh->geometry_count; ++i) {
		SGeometryConfig& Config = Configs[i];
		LoadParams.out_mesh->geometries[i] = GeometrySystem::Get().AcquireFromConfig(Config, true);
	}
	LoadParams.out_mesh->Generation++;

	GLOG(Log::eInfo, "Successfully loaded mesh: '%s'.", LoadParams.resource_name.CStr());
	ResourceSystem::Get().Unload(&LoadParams.mesh_resource);
}

void AStaticMeshActor::LoadJobFail() {
	GLOG(Log::eError, "Failed to load mesh: '%s'.", LoadParams.resource_name.CStr());
	ResourceSystem::Get().Unload(&LoadParams.mesh_resource);
}

bool AStaticMeshActor::LoadJobStart() {
	bool Result = ResourceSystem::Get().Load(Name_, EAssetType::StaticMesh, nullptr, &LoadParams.mesh_resource);
	return Result;
}

bool AStaticMeshActor::LoadFromResource(const FString& resource_name) {
	Generation = INVALID_ID_U8;

	LoadParams.resource_name = resource_name;
	LoadParams.out_mesh = this;
	LoadParams.mesh_resource = {};
	Name_ = resource_name;

	JobInfo Job;
	Job.entry = [this]() {return LoadJobStart(); };
	Job.on_success = [this]() {return LoadJobSuccess(); };
	Job.on_failed = [this]() {return LoadJobFail(); };
	Job.type = JobType::eResource_Load;

	JobSystem::Submit(Job);

	return true;
}

void AStaticMeshActor::Unload() {
	for (uint32_t i = 0; i < geometry_count; ++i) {
		GeometrySystem::Get().Release(geometries[i]);
	}

	Memory::Free(geometries, MemoryType::eMemory_Type_Array);
	geometries = nullptr;

	// For good measure. Invalidate the geometry so it doesn't attemp to be renderer.
	geometry_count = 0;
	Generation = INVALID_ID_U8;
}
