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
	RenderWorldInstance = NewObject<URenderWorld>();
	if (!RenderWorldInstance) {
		return false;
	}


	return true;
}

void UWorld::Tick(float DeltaTime) {
	for (AActor* Actor : WorldActors) {
		if (Actor && Actor->IsEnableTick()) {
			Actor->Tick(DeltaTime);
		}
	}
}

void UWorld::Destroy() {
	if (RenderWorldInstance) {
		DeleteObject(RenderWorldInstance);
		RenderWorldInstance = nullptr;
	}
}

void UWorld::AddActor(AActor* Actor) {
	Actor->SetWorld(this);
	Actor->RegisterComponents();
	if (!Actor) return;
	WorldActors.Push(Actor);
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
