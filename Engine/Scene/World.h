#pragma once

#include "Framework/Object.h"
#include "Containers/TArray.hpp"

class AActor;
class URenderWorld;
class UWidget;

class ENGINE_API UWorld : public UObject {
	DECLARE_CLASS_TYPE(UWorld)

public:
	UWorld();
	virtual ~UWorld();

	virtual void PreInitialize() override {};
	virtual bool Initialize() override;
	virtual void PostInitialize() override {};
	virtual void RegisterActors();
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);

	virtual void Destroy();

	URenderWorld* GetRenderWorld() const { return RenderWorldInstance; }

public:
	void AddActor(AActor* Actor);
	void RemoveActor(AActor* Actor);
	void ClearActor();

	// UI 根列表：UWorld 每帧驱动 测量 -> 布局 -> 绘制 -> 提交代理。
	// World 只驱动生命周期（TickUI / UnregisterUI），不接管 Widget 树所有权
	// （所有权归创建者，通常为持有该根的 Actor）。
	void AddUIWidgetRoot(UWidget* Root);
	void RemoveUIWidgetRoot(UWidget* Root);

	const TArray<AActor*>& GetWorldActors() const { return WorldActors; }

	uint64_t GetVisibleGeometryCount() const;

protected:
	bool IsRunning = false;
	TArray<AActor*> WorldActors;
	TArray<UWidget*> UIWidgetRoots;
	URenderWorld* RenderWorldInstance;
};