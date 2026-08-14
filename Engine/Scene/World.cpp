#include "World.h"
#include "Rendering/RenderWorld/RenderWorld.h"
#include "Framework/Classes/Actor.h"

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

	if (RenderWorldInstance) {
		DeleteObject(RenderWorldInstance);
		RenderWorldInstance = nullptr;
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
