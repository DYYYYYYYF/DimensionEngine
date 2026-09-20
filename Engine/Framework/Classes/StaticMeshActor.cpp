#include "StaticMeshActor.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Systems/ResourceSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/JobSystem.hpp"
#include "Rendering/RenderTypes.hpp"
#include "Framework/Components/StaticMeshComponent.h"

AStaticMeshActor::AStaticMeshActor(const FString& Name) 
	: AActor(Name), GeometryAsset(nullptr), geometry_count(0), Generation(INVALID_ID_U8), LoadState(nullptr)
{
	MeshComponent = CreateComponent<UStaticMeshComponent>("MeshComponent");
	if (MeshComponent) {
		SetRootComponent(MeshComponent);
	}

}

// 静态加载入口（worker 线程执行）：只操作共享状态（原子）与局部资源容器，绝不写 Actor 成员，
// 避免与主线程 Unload/DeleteObject(GeometryAsset) 并发导致 use-after-free。
bool AStaticMeshActor::LoadJobStart(const FString& Name, std::shared_ptr<LoadJobState> State) {
	// 若 Actor 已被 Unload/销毁，直接放弃加载
	if (!State || State->bInvalid.load(std::memory_order_acquire)) {
		return false;
	}

	UGeometry* LoadingAsset = NewObject<UGeometry>(Name);
	bool Result = ResourceSystem::Get().Load(Name, EAssetType::StaticMesh, nullptr, LoadingAsset);
	if (!Result) {
		DeleteObject(LoadingAsset);
		return false;
	}

	// Load 完成后再检查一次：期间 Actor 被销毁则就地释放，防止泄漏
	if (State->bInvalid.load(std::memory_order_acquire)) {
		// 必须先 Unload 释放 Data（顶点/索引内存），再删对象：
		// DeleteObject 的析构不释放 UAsset::Data，直接删会造成每轮约 14MiB 泄漏
		ResourceSystem::Get().Unload(LoadingAsset);
		DeleteObject(LoadingAsset);
		return false;
	}

	State->Asset.store(LoadingAsset, std::memory_order_release);
	return true;
}

void AStaticMeshActor::LoadJobSuccess(UGeometry* Asset) {
	FGeometryConfig* Configs = (FGeometryConfig*)Asset->Data;

	TArray<UGeometry*> Mesh;
	for (uint32_t i = 0; i < Asset->DataCount; ++i) {
		FGeometryConfig& Config = Configs[i];
		UGeometry* NewGeometry = GeometrySystem::Get().AcquireFromConfig(Config, true);
		if (NewGeometry) Mesh.Push(NewGeometry);
	}
	Asset->Generation++;

	GLOG(Log::eInfo, "Successfully loaded mesh: '%s'.", Asset->GetName().CStr());
	ResourceSystem::Get().Unload(Asset);

	// 主线程（本回调）赋给 Actor 成员；Unload 也在主线程释放，无数据竞争
	GeometryAsset = Asset;

	// 更新Proxy
	MeshComponent->SetMesh(Mesh);
	MeshComponent->UpdateRenderProxy();
}

void AStaticMeshActor::LoadJobFail() {
	GLOG(Log::eError, "Failed to load mesh resource (actor: '%s').", GetName().CStr());
}

bool AStaticMeshActor::LoadFromResource(const FString& resource_name) {
	Generation = INVALID_ID_U8;
	LoadState = std::make_shared<LoadJobState>();

	JobInfo Job;
	// entry 不捕获 this：worker 线程只操作共享状态，避免访问已销毁的 Actor
	Job.entry = [Name = FString(resource_name), State = LoadState]() { return LoadJobStart(Name, State); };
	// 回调捕获 this，但先检查失效标记：仅当 Actor 仍存活（主线程串行保证）才解引用 this；
	// Actor 已销毁时仅释放残留资源容器，不触碰 Actor 任何成员。
	Job.on_success = [this, State = LoadState]() {
		UGeometry* Asset = State->Asset.exchange(nullptr, std::memory_order_acq_rel);
		if (State->bInvalid.load(std::memory_order_acquire)) {
			// Actor 已被销毁：先 Unload 释放 Data 再删对象，防止 Data 泄漏
			if (Asset) {
				ResourceSystem::Get().Unload(Asset);
				DeleteObject(Asset);
			}
			return;
		}
		if (Asset) this->LoadJobSuccess(Asset);
	};
	Job.on_failed = [this, State = LoadState]() {
		UGeometry* Asset = State->Asset.exchange(nullptr, std::memory_order_acq_rel);
		if (State->bInvalid.load(std::memory_order_acquire)) {
			if (Asset) {
				ResourceSystem::Get().Unload(Asset);
				DeleteObject(Asset);
			}
			return;
		}
		if (Asset) {
			ResourceSystem::Get().Unload(Asset);
			DeleteObject(Asset);
		}
		this->LoadJobFail();
	};
	Job.type = JobType::eResource_Load;
	// 取消谓词：Actor 被 Unload/销毁（bInvalid=true）后，队列满时该 job 可被优先淘汰、
	// worker 取出后也会直接跳过，避免无意义加载与无效回调堆积。
	Job.is_cancelled = [State = LoadState]() {
		return State->bInvalid.load(std::memory_order_acquire);
	};

	if (!JobSystem::Submit(Job)) {
		// 队列满被拒绝（系统未运行时也走这里）：置失效避免回调悬挂，调用方降级处理。
		LoadState->bInvalid.store(true, std::memory_order_release);
		GLOG(Log::eWarn, "AStaticMeshActor: load job rejected (queue full), actor '%s' load aborted.",
			resource_name.CStr());
		return false;
	}

	return true;
}

void AStaticMeshActor::Unload() {
	// 置失效标记：后续 LoadJobStart（worker）与回调（主线程）据此放弃对资源的访问，
	// 必须在释放 GeometryAsset 之前执行
	if (LoadState) {
		LoadState->bInvalid.store(true, std::memory_order_release);
	}

	const TArray<UGeometry*>& Submeshes = MeshComponent->GetGeometries();
	for (uint32_t i = 0; i < Submeshes.Size(); ++i) {
		if (!Submeshes[i]) continue;
		Submeshes[i]->DecreaseReferenceCount();
	}

	// 区分 GeometryAsset 的来源，避免错误释放：
	//  - 模型路径（LoadJobStart 中 NewObject 创建的资源容器，不在 Submeshes 内）：用 DeleteObject 释放对象本身；
	//  - Cube 路径（GeometrySystem::AcquireFromConfig 返回的系统几何，注册表持有）：归系统统一管理，禁止裸 Free。
	// 注意：bIsSystemGeometry 判断必须在本函数内对 Submeshes 的遍历完成之后（ClearGeometries 会清空数组）
	bool bIsSystemGeometry = false;
	if (GeometryAsset) {
		for (uint32_t i = 0; i < Submeshes.Size(); ++i) {
			if (Submeshes[i] == GeometryAsset) {
				bIsSystemGeometry = true;
				break;
			}
		}
	}
	if (GeometryAsset && !bIsSystemGeometry) {
		DeleteObject(GeometryAsset);
	}
	GeometryAsset = nullptr;

	// 清空几何数组：避免后续新的 LoadJobSuccess 调 SetMesh 时，
	// 对这批已 Decrease（可能已销毁）的悬垂几何再次 DecreaseReferenceCount 造成 Use-After-Free
	MeshComponent->ClearGeometries();

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
