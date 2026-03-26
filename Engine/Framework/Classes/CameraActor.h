#pragma once

#include "Actor.h"

class UCameraComponent;

class ENGINE_API ACameraActor : public AActor {
public:
    DECLARE_CLASS_TYPE(ACameraActor)

public:
    ACameraActor();
    explicit ACameraActor(const FString& Name);
    virtual ~ACameraActor() = default;

public:
    //  Actor 生命周期
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Destroy() override;

    UCameraComponent* GetCameraComponent() { return CameraComponent; }

public:
    // 引用次数
	uint32_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

private:
    uint32_t ReferenceCount = 0;
    UCameraComponent* CameraComponent;
};