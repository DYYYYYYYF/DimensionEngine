#pragma once

#include "GeometryType.hpp"
#include "Rendering/Resources/Asset.hpp"

class Material;

class DAPI Geometry : public UAsset {
	friend class GeometrySystem;

public:
	Geometry();
	Geometry(const FString& name);
	virtual ~Geometry();

public:
	inline size_t GetReferenceCount() const { return ReferenceCount; }
	inline void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1);

	inline bool IsAutoRelease() const { return AutoRelease; }
	inline void SetIsAutoRelease(bool b) { AutoRelease = b; }

	Material* GetMaterial() const { return Material; }

private:
	void DestroyInstance();

public:
	uint32_t ID;
	uint32_t InternalID;	// Renderer内部使用的ID，是Vulkan的Buffer ID
	uint32_t Generation;
	Vector3 Center;
	Extents3D Extents;
	FString name;
	Material* Material = nullptr;

private:
	size_t ReferenceCount = 0;
	bool AutoRelease = true;
};