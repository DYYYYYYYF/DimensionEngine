#pragma once

#include "Framework/Object.h"
#include "Containers/TArray.hpp"

class AActor;
class URenderWorld;

class ENGINE_API UWorld : public UObject {
	DECLARE_CLASS_TYPE(UWorld)

public:
	UWorld();
	virtual ~UWorld();

	virtual void PreInitialize() override {};
	virtual bool Initialize() override;
	virtual void PostInitialize() override {};

	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);

	virtual void Destroy();

	URenderWorld* GetRenderWorld() const { return RenderWorldInstance; }

public:
	void AddActor(AActor* Actor);
	void RemoveActor(AActor* Actor);
	void ClearActor();

	const TArray<AActor*>& GetWorldActors() const { return WorldActors; }

	uint64_t GetVisibleGeometryCount() const;

protected:
	TArray<AActor*> WorldActors;
	URenderWorld* RenderWorldInstance;
};