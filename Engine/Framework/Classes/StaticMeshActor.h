#pragma once

#include "Actor.h"
#include "Rendering/Resources/Geometry/Geometry.hpp"

#include <atomic>
#include <memory>
#include <vector>

// C4251: std::shared_ptr 等 STL 类型成员跨 DLL 接口为 MSVC 已知良性警告（同一 CRT），局部抑制
#pragma warning(push)
#pragma warning(disable : 4251)

class UGeometry;
class UStaticMeshComponent;

class ENGINE_API AStaticMeshActor : public AActor{
public:
	DECLARE_CLASS_TYPE(AStaticMeshActor)

	// 异步加载共享状态：worker 线程只读写该状态（原子字段），主线程回调与 Unload 也读写它。
	// 通过 bInvalid 标记让回调感知 Actor 是否已被 Unload/销毁，杜绝：
	//  1) worker 线程向已释放的 Actor 成员 GeometryAsset 写入（use-after-free）；
	//  2) 回调在 Actor 销毁后读取已释放 GeometryAsset->Data（垃圾 FGeometryConfig 导致
	//     DynamicAllocator::AllocateAligned 的 RequiredSize 溢出断言崩溃）。
	struct LoadJobState {
		std::atomic<bool>       bInvalid{ false };
		std::atomic<UGeometry*> Asset{ nullptr };
	};

public:
	AStaticMeshActor(const FString& Name);
	virtual ~AStaticMeshActor() { Unload(); }

public:
	bool LoadFromResource(const FString& resource_name);
	void Unload();

	bool SetMeshResource(UGeometry* mesh_resource);

private:
	static bool LoadJobStart(const FString& Name, std::shared_ptr<LoadJobState> State);
	void LoadJobSuccess(UGeometry* Asset);
	void LoadJobFail();

public:
	unsigned char Generation;
	unsigned short geometry_count;
	UGeometry* GeometryAsset;
	std::shared_ptr<LoadJobState> LoadState;

	UStaticMeshComponent* MeshComponent = nullptr;
};
#pragma warning(pop)
