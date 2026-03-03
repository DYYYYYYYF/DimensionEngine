#pragma once

#include "Actor.h"

/**
 * @brief 相机 Actor，直接继承 Actor，内化原 Camera 类的全部逻辑。
 *
 * 旋转以弧度存储在 EulerRotation（pitch=x, yaw=y, roll=z），
 * ViewMatrix 按需重建（IsDirty 标志驱动），
 * 位置和旋转同时维护在 Actor::LocalTransform，保持变换系统统一。
 */
class ENGINE_API ACameraActor : public Actor {
public:
    ACameraActor();
    explicit ACameraActor(const std::string& Name);
    virtual ~ACameraActor() = default;

public:
    // ----------------------------------------------------------------
    //  Actor 生命周期
    // ----------------------------------------------------------------
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Destroy() override;

public:
    // ----------------------------------------------------------------
    //  位置（覆盖 Actor 基类，额外标记 IsDirty）
    // ----------------------------------------------------------------
    void    SetPosition(const Vector3& Pos);
    Vector3 GetPosition() const { return LocalTransform.GetLocation(); }

    // ----------------------------------------------------------------
    //  旋转 —— Euler 角度制（与原 Camera 接口保持一致）
    // ----------------------------------------------------------------
    void    SetEulerAngles(const Vector3& EulerDeg);
    Vector3 GetEulerAngles() const;

    // ----------------------------------------------------------------
    //  ViewMatrix
    // ----------------------------------------------------------------
    Matrix4 GetViewMatrix();
    void    SetViewMatrix(const Matrix4& Mat);

    // ----------------------------------------------------------------
    //  移动
    // ----------------------------------------------------------------
    void MoveForward(float Amount);
    void MoveBackward(float Amount);
    void MoveLeft(float Amount);
    void MoveRight(float Amount);
    void MoveUp(float Amount);
    void MoveDown(float Amount);

    // ----------------------------------------------------------------
    //  旋转
    // ----------------------------------------------------------------
    void RotateYaw(float Amount);   // 绕 Y 轴，传入弧度增量
    void RotatePitch(float Amount);   // 绕 X 轴，传入弧度增量，内部限幅

    // ----------------------------------------------------------------
    //  方向向量（从 ViewMatrix 读取）
    // ----------------------------------------------------------------
    Vector3 Forward() { return GetViewMatrix().Forward(); }
    Vector3 Backward() { return GetViewMatrix().Backward(); }
    Vector3 Left() { return GetViewMatrix().Left(); }
    Vector3 Right() { return GetViewMatrix().Right(); }
    Vector3 Up() { return GetViewMatrix().Up(); }

    // ----------------------------------------------------------------
    //  重置
    // ----------------------------------------------------------------
    void Reset();

    // 引用次数
	uint32_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

private:
    // ViewMatrix 重建（当 IsDirty_ 时调用）
    void RebuildViewMatrix();

    // 将当前 EulerRotation_/Position 同步写入 LocalTransform
    void SyncToTransform();

private:
    /**
     * @brief The reference count of this camera.
     */
    uint32_t ReferenceCount = 0;

    // 旋转以弧度存储（pitch=x, yaw=y, roll=z），是唯一旋转真相来源
    Vector3 EulerRotation_{ 0.0f, 0.0f, 0.0f };

    // 缓存的 ViewMatrix
    Matrix4 ViewMatrix_;

    // ViewMatrix 是否需要重建
    bool IsDirty_ = true;

    // Pitch 限幅（弧度），避免万向锁，约 ±89°
    static constexpr float PitchLimit = 1.55334306f;
};