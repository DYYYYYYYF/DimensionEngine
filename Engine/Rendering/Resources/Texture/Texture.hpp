#pragma once

#include "TextureType.hpp"
#include "Rendering/Resources/Asset.hpp"
#include "Math/MathTypes.hpp"

class UTexture : public UAsset {
public:
	UTexture();
	virtual ~UTexture();

public:
	uint32_t GetID() const { return ID; }
	void SetID(uint32_t id) { ID = id; }

	size_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

	bool IsAutoRelease() const { return AutoRelease; }
	void SetIsAutoRelease(bool b) { AutoRelease = b; }

	const FString& GetName() const { return Name; }
	void SetName(const FString& n) { Name = n; }

	uint32_t GetWidth() const { return Width; }
	uint32_t GetHeight() const { return Height; }

private:
	uint32_t ID;
	FString Name;
	size_t ReferenceCount;
	bool AutoRelease;

public:
	TextureType Type = TextureType::eTexture_Type_2D;
	uint32_t Width = 100;
	uint32_t Height = 100;

	int ChannelCount = 4;
	/** TextureFlag */
	TextureFlag Flags = 0;

	uint32_t Generation = INVALID_ID;
	void* InternalData;
};
