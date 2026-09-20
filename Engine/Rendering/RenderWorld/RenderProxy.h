#pragma once

#include "Math/MathTypes.hpp"
#include "Containers/TArray.hpp"

class UGeometry;

enum ERenderFeature : uint32_t {
	None = 0,
	ForwardLighting = 1 << 1,
	DeferredLighting = 1 << 2,
	UI = 1 << 3,
	Lighting = 1 << 4,
	Skybox = 1 << 5,
};

// 渲染代理静态类型标识。注意 Cast 宏为 static_cast，无法做动态类型判断，
// RenderView 必须据此分支，避免把 FUIRenderProxy 误判为文本代理等。
enum ERenderProxyType : uint32_t {
	ProxyType_StaticMesh,
	ProxyType_Text,
	ProxyType_UI,
	ProxyType_Skybox,
};

class FRenderProxy {
public:
	FRenderProxy(ERenderProxyType InType = ERenderProxyType::ProxyType_StaticMesh)
		: ProxyType(InType), bVisible(true), DistanceToCamera(0.0f), UniqueID(INVALID_ID), ModelMatrix(Matrix4::Identity()){}

	ERenderProxyType GetProxyType() const { return ProxyType; }

	void SetModelMatrix(const Matrix4& InModelMatrix) { ModelMatrix = InModelMatrix; }
	const Matrix4& GetModelMatrix() const { return ModelMatrix; }

	void SetVisibility(bool bInVisible) { bVisible = bInVisible; }
	bool IsVisible() const { return bVisible; }

	void SetDistanceToCamera(float InDistance) { DistanceToCamera = InDistance; }
	float GetDistanceToCamera() const { return DistanceToCamera; }

	void SetUniqueID(uint64_t InUniqueID) { UniqueID = InUniqueID; }
	uint64_t GetUniqueID() const { return UniqueID; }

	void SetRenderFeatureFlags(uint32_t InFlags) { RenderFeatureFlags |= InFlags; }
	uint32_t GetRenderFeatureFlags() const { return RenderFeatureFlags; }

protected:
	ERenderProxyType ProxyType;
	uint32_t RenderFeatureFlags = ERenderFeature::None; // Bitmask for render features
	Matrix4 ModelMatrix;   // Transformation matrix for the proxy
	bool bVisible;        // Visibility flag for rendering
	float DistanceToCamera; // Distance from the camera for sorting purposes
	uint64_t UniqueID;
};

class FStaticMeshRenderProxy : public FRenderProxy {
public:
	FStaticMeshRenderProxy() : FRenderProxy(ERenderProxyType::ProxyType_StaticMesh) { RenderFeatureFlags |= ERenderFeature::DeferredLighting; }

	void SetMesh(TArray<UGeometry*> InMesh) { Mesh = InMesh; }
	TArray<UGeometry*> GetMesh() const { return Mesh; }

	void SetBoundingBox(const Extents3D& InBoundingBox) { BoundingBox = InBoundingBox; }
	const Extents3D& GetBoundingBox() const { return BoundingBox; }

protected:
	TArray<UGeometry*> Mesh;
	Extents3D BoundingBox; // Axis-aligned bounding box for culling

};

class FTextRenderProxy : public FRenderProxy {
public:
	FTextRenderProxy() : FRenderProxy(ERenderProxyType::ProxyType_Text), Mesh(nullptr) { RenderFeatureFlags |= ERenderFeature::UI; }

	void SetMesh(UGeometry* InMesh) { Mesh = InMesh; }
	UGeometry* GetMesh() const { return Mesh; }

protected:
	UGeometry* Mesh;

};

// UI 元素渲染代理：由 Widget 树的 UCanvasRenderer 收集的 UGeometry 列表
class FUIRenderProxy : public FRenderProxy {
public:
	FUIRenderProxy() : FRenderProxy(ERenderProxyType::ProxyType_UI) { RenderFeatureFlags |= ERenderFeature::UI; }

	void SetMesh(const TArray<UGeometry*>& InMesh) { Mesh = InMesh; }
	const TArray<UGeometry*>& GetMesh() const { return Mesh; }

	// 内容代次：UI 组件每成功重建一次合批几何递增。RenderViewUI 据此判断静态
	// DrawCall 缓存是否仍有效（R4：静止 UI 帧跳过每帧 DrawCall 重建与排序）。
	void SetContentGeneration(uint64_t InGeneration) { ContentGeneration = InGeneration; }
	uint64_t GetContentGeneration() const { return ContentGeneration; }

protected:
	TArray<UGeometry*> Mesh;
	uint64_t ContentGeneration = 0;

};

class FSkyboxRenderProxy : public FRenderProxy {
public:
	FSkyboxRenderProxy() : FRenderProxy(ERenderProxyType::ProxyType_Skybox), Mesh(nullptr) { RenderFeatureFlags |= ERenderFeature::Skybox; }

	void SetMesh(UGeometry* InMesh) { Mesh = InMesh; }
	UGeometry* GetMesh() const { return Mesh; }

protected:
	UGeometry* Mesh;

};