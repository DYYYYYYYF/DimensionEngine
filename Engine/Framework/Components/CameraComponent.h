#pragma once
#include "SceneComponent.h"
#include "Math/MathTypes.hpp"

class DAPI UCameraComponent : public USceneComponent {
	DECLARE_CLASS_TYPE(UCameraComponent)

public:
	UCameraComponent(const FString& Name);

	virtual void OnTransformChanged() override;

public:
	//  位置（覆盖 Actor 基类，额外标记 IsDirty）
	void    SetPosition(const Vector3& Pos);
	Vector3 GetPosition() const;

	//  旋转 —— Euler 角度制（与原 Camera 接口保持一致）
	void    SetEulerAngles(const Vector3& EulerDeg);
	Vector3 GetEulerAngles() const;

	//  ViewMatrix
	Matrix4 GetViewMatrix() const;
	void    SetViewMatrix(const Matrix4& Mat);

	//  移动
	void MoveForward(float Amount);
	void MoveBackward(float Amount);
	void MoveLeft(float Amount);
	void MoveRight(float Amount);
	void MoveUp(float Amount);
	void MoveDown(float Amount);

	//  旋转
	void RotateYaw(float Amount);   // 绕 Y 轴，传入弧度增量
	void RotatePitch(float Amount);   // 绕 X 轴，传入弧度增量，内部限幅

	//  方向向量（从 ViewMatrix 读取）
	Vector3 Forward() const { return GetViewMatrix().Forward(); }
	Vector3 Backward() const { return GetViewMatrix().Backward(); }
	Vector3 Left() const { return GetViewMatrix().Left(); }
	Vector3 Right() const { return GetViewMatrix().Right(); }
	Vector3 Up() const { return GetViewMatrix().Up(); }

	// 基础信息
	void SetFOV(float FOV) { FOV_ = FOV; }
	float GetFOV() const { return FOV_; }
	void SetAspectRatio(float AspectRatio) { AspectRatio_ = AspectRatio; }
	float GetAspectRatio() const { return AspectRatio_; }
	void SetNearPlane(float NearPlane) { NearPlane_ = NearPlane; }
	float GetNearPlane() const { return NearPlane_; }
	void SetFarPlane(float FarPlane) { FarPlane_ = FarPlane; }
	float GetFarPlane() const { return FarPlane_; }

	// 获取视锥体
	const FFrustum& GetFrustum() const;
	void UpdateFrustum() const;

	//  重置
	void Reset();

protected:
	// ViewMatrix 重建（当 IsDirty_ 时调用）
	void RebuildViewMatrix() const;

	// 将当前 EulerRotation_/Position 同步写入 LocalTransform
	void SyncToTransform();


protected:
	// 基础设置
	float FOV_ = 60.0f;   // 视野角度（度）
	float AspectRatio_ = 16.0f / 9.0f;   // 宽高比
	float NearPlane_ = 0.1f;   // 近平面
	float FarPlane_ = 1000.0f;   // 远平面

	// 视锥体
	mutable FFrustum Frustum_;
	mutable bool IsBaseDataDirty_ = true;

	// 旋转以弧度存储（pitch=x, yaw=y, roll=z），是唯一旋转真相来源
	Vector3 EulerRotation_{ 0.0f, 0.0f, 0.0f };
	// 缓存的 ViewMatrix
	mutable Matrix4 ViewMatrix_;
	// ViewMatrix 是否需要重建
	mutable bool IsDirty_ = true;

	// Pitch 限幅（弧度），避免万向锁，约 ±89°
	static constexpr float PitchLimit = 1.55334306f;
};