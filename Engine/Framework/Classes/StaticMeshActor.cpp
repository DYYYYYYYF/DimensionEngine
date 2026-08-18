#include "StaticMeshActor.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/JobSystem.hpp"
#include "Rendering/RenderTypes.hpp"
#include "Framework/Components/StaticMeshComponent.h"

AStaticMeshActor::AStaticMeshActor(const FString& Name) 
	: AActor(Name), GeometryAsset(nullptr), geometry_count(0), Generation(INVALID_ID_U8)
{
	MeshComponent = CreateComponent<UStaticMeshComponent>("MeshComponent");
	if (MeshComponent) {
		SetRootComponent(MeshComponent);
	}

}

void AStaticMeshActor::LoadJobSuccess() {
	FGeometryConfig* Configs = (FGeometryConfig*)GeometryAsset->Data;

	TArray<UGeometry*> Mesh;
	for (uint32_t i = 0; i < GeometryAsset->DataCount; ++i) {
		FGeometryConfig& Config = Configs[i];
		UGeometry* NewGeometry = GeometrySystem::Get().AcquireFromConfig(Config, true);
		if (NewGeometry) Mesh.Push(NewGeometry);
	}
	GeometryAsset->Generation++;

	GLOG(Log::eInfo, "Successfully loaded mesh: '%s'.", GeometryAsset->GetName().CStr());
	ResourceSystem::Get().Unload(GeometryAsset);

	// 更新Proxy
	MeshComponent->SetMesh(Mesh);
	MeshComponent->UpdateRenderProxy();
}

void AStaticMeshActor::LoadJobFail() {
	GLOG(Log::eError, "Failed to load mesh: '%s'.", GeometryAsset->GetName().CStr());
	ResourceSystem::Get().Unload(GeometryAsset);
}

bool AStaticMeshActor::LoadJobStart() {
	GeometryAsset = NewObject<UGeometry>(Name_);
	bool Result = ResourceSystem::Get().Load(Name_, EAssetType::StaticMesh, nullptr, GeometryAsset);
	return Result;
}

bool AStaticMeshActor::LoadFromResource(const FString& resource_name) {
	Generation = INVALID_ID_U8;

	JobInfo Job;
	Job.entry = [this]() {return LoadJobStart(); };
	Job.on_success = [this]() {return LoadJobSuccess(); };
	Job.on_failed = [this]() {return LoadJobFail(); };
	Job.type = JobType::eResource_Load;

	JobSystem::Submit(Job);

	return true;
}

void AStaticMeshActor::Unload() {
	const TArray<UGeometry*>& Submeshes = MeshComponent->GetGeometries();
	for (uint32_t i = 0; i < Submeshes.Size(); ++i) {
		if (!Submeshes[i]) continue;
		Submeshes[i]->DecreaseReferenceCount();
	}

	Memory::Free(GeometryAsset, MemoryType::eMemory_Type_Array);
	GeometryAsset = nullptr;

	// For good measure. Invalidate the geometry so it doesn't attemp to be renderer.
	geometry_count = 0;
	Generation = INVALID_ID_U8;
}

bool AStaticMeshActor::SetMeshResource(UGeometry* mesh_resource)
{
	if (!MeshComponent) return false;

	MeshComponent->SetMesh({mesh_resource});
	return true;
}
