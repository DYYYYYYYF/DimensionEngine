#include "World.h"
#include "Rendering/RenderWorld/RenderWorld.h"
#include "Framework/Classes/Actor.h"
#include "UI/Widget.h"

UWorld::UWorld() {
	RenderWorldInstance = NewObject<URenderWorld>(MemoryType::eMemory_Type_Renderer);
	ASSERT(RenderWorldInstance);
}

UWorld::~UWorld() {
	Destroy();
}

bool UWorld::Initialize() {
	// Preinitialize
	for (AActor* Actor : WorldActors) {
		if (Actor) {
			Actor->PreInitialize();
		}
	}

	// Initialize
	for (AActor* Actor : WorldActors) {
		if (Actor) {
			Actor->Initialize();
		}
	}

	// Postinitialize
	for (AActor* Actor : WorldActors) {
		if (Actor) {
			Actor->PostInitialize();
		}
	}

	return true;
}

void UWorld::RegisterActors() {
	for (AActor* Actor : WorldActors) {
		if (Actor) {
			Actor->RegisterComponents();
		}
	}
}

void UWorld::BeginPlay() {
	// 初始化之后在进行Register，这时Mesh已经生成
	RegisterActors();

	for (AActor* Actor : WorldActors) {
		if (Actor && Actor->IsEnableTick()) {
			Actor->BeginPlay();
		}
	}

	IsRunning = true;
}

void UWorld::Tick(float DeltaTime) {
	for (AActor* Actor : WorldActors) {
		if (Actor && Actor->IsEnableTick()) {
			Actor->Tick(DeltaTime);
		}
	}

	// UI 根生命周期：每帧驱动 测量 -> 布局 -> 绘制 -> 提交渲染代理。
	// 放在 Actor Tick 之后、RenderWorld Record 之前，保证 UI 元素
	// 在帧末渲染时已就绪（保持独立 UI 渲染通道不变）。
	for (UWidget* Root : UIWidgetRoots) {
		if (Root) {
			Root->TickUI(DeltaTime);
		}
	}
}

void UWorld::Destroy() {
	// 移除所有Actor
	for (AActor* Actor : WorldActors) {
		// 从场景中移除
		Actor->UnregisterComponents();

		// 清空内存
		Actor->Destroy();
		DeleteObject(Actor);
		Actor = nullptr;
	}
	WorldActors.Empty();

	// UI 根注销（RenderWorld 仍存活，保证 RemoveProxy 有效）。
	// 所有权归创建者（通常为 Actor），World 只负责在销毁前注销渲染代理，
	// 不 delete Widget 树；若创建者尚未清理，根节点析构/ReRegister 会自行处理。
	for (UWidget* Root : UIWidgetRoots) {
		if (Root) {
			Root->UnregisterUI();
		}
	}
	UIWidgetRoots.Empty();

	if (RenderWorldInstance) {
		DeleteObject(RenderWorldInstance);
		RenderWorldInstance = nullptr;
	}
}

void UWorld::AddUIWidgetRoot(UWidget* Root) {
	if (!Root) return;
	Root->SetUIWorld(this);
	UIWidgetRoots.Push(Root);
}

void UWorld::RemoveUIWidgetRoot(UWidget* Root) {
	if (!Root) return;
	Root->UnregisterUI();
	for (uint32_t i = 0; i < (uint32_t)UIWidgetRoots.Size(); ++i) {
		if (UIWidgetRoots[i] && UIWidgetRoots[i] == Root) {
			UIWidgetRoots.PopAt(i);
			break;
		}
	}
}

void UWorld::AddActor(AActor* Actor) {
	if (!Actor) return;
	Actor->SetWorld(this);
	WorldActors.Push(Actor);

	// 如果已经在运行中则直接注册
	if (IsRunning) {
		Actor->RegisterComponents();
	}
}

void UWorld::RemoveActor(AActor* Actor) {
	for (uint32_t i = 0; i < (uint32_t)WorldActors.Size(); ++i) {
		if (WorldActors[i] && WorldActors[i] == Actor) {
			WorldActors[i]->UnregisterComponents();
			WorldActors.PopAt(i);
			break;
		}
	}
}

void UWorld::ClearActor() {
	WorldActors.Clear();
}

uint64_t UWorld::GetVisibleGeometryCount() const {
	URenderWorld* RenderViewWorld = GetRenderWorld();
	if (!RenderViewWorld) {
		return 0;
	}

	return RenderViewWorld->GetVisibleGeometryCount();
}
