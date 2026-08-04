#pragma once

#include "GeometryType.hpp"
#include "Rendering/Resources/Asset.hpp"

class UMaterial;
class UMaterialInstance;

class DAPI UGeometry : public UAsset {
	friend class GeometrySystem;

public:
	UGeometry(const FString& name);
	virtual ~UGeometry();

public:
	void SetMaterial(UMaterial* Mat);

	inline size_t GetReferenceCount() const { return ReferenceCount; }
	inline void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1);

	inline bool IsAutoRelease() const { return AutoRelease; }
	inline void SetIsAutoRelease(bool b) { AutoRelease = b; }

	UMaterialInstance* GetMaterialInstance() const { return MaterialInstance; }

private:
	void DestroyInstance();

public:
	uint32_t ID;
	uint32_t InternalID;	// Renderer内部使用的ID，是Vulkan的Buffer ID
	uint32_t Generation;
	Vector3 Center;
	Extents3D Extents;
	FString name;

protected:
	UMaterialInstance* MaterialInstance = nullptr;

private:
	size_t ReferenceCount = 0;
	bool AutoRelease = true;

};