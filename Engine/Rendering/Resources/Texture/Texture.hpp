#pragma once

#include "TextureType.hpp"
#include "Rendering/Resources/Asset.hpp"
#include "Math/MathTypes.hpp"

class UTexture : public UAsset {
public:
	UTexture();
	UTexture(const FString& name);
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

public:
	void SetName(const FString& n) { Name = n; }
	const FString& GetName() const { return Name; }

	void SetWidth(uint32_t W) { Width = W; }
	uint32_t GetWidth() const { return Width; }
	void SetHeight(uint32_t H) { Height = H; }
	uint32_t GetHeight() const { return Height; }

	uint32_t GetSize() const { return GetWidth() * GetHeight() * GetChannelCount(); }

	void SetTextureType(TextureType t) { Type = t; }
	TextureType GetTextureType() const { return Type; }
	void SetChannelCount(uint32_t RequiredChannelCount) { ChannelCount = RequiredChannelCount; }
	uint32_t GetChannelCount() const { return ChannelCount; }

	void SetPixels(unsigned char* raw) { Pixels = raw; }
	unsigned char* GetPixels() const { return Pixels; }

	void SetFlags(TextureFlag f) { Flags = f; }
	TextureFlag GetFlags() const { return Flags; }

private:
	uint32_t ID;
	size_t ReferenceCount;
	bool AutoRelease;

public:
	TextureType Type = TextureType::eTexture_Type_2D;
	uint32_t Width = 100;
	uint32_t Height = 100;
	uint32_t ChannelCount = 4;

	/** TextureFlag */
	TextureFlag Flags = 0;

	uint32_t Generation = INVALID_ID;

	unsigned char* Pixels = nullptr;
};
