#pragma once

#include "Actor.h"

class UCameraComponent;

enum class ECameraProjectionMode {
	Perspective,
	Orthographic
};

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

    UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	// 设置摄像机组件
	void SetFOV(float FOV);
	float GetFOV() const;
	void SetAspectRatio(float AspectRatio);
	float GetAspectRatio() const;
	void SetNearPlane(float NearPlane);
	float GetNearPlane() const;
	void SetFarPlane(float FarPlane);
	float GetFarPlane() const;

	Matrix4 GetProjectionMatrix(ECameraProjectionMode Mode = ECameraProjectionMode::Perspective) const;
	Matrix4 GetViewMatrix() const;

	const FFrustum& GetFrustum() const;

public:
    // 引用次数
	uint32_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

private:
	// 其他
    uint32_t ReferenceCount = 0;
    UCameraComponent* CameraComponent;
};