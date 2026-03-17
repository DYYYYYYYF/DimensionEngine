#pragma once

#include "TextureType.hpp"
#include "Rendering/Resources/Asset.hpp"
#include "Math/MathTypes.hpp"
#include "Math/Color.hpp"

class UTexture : public UAsset {
public:
	UTexture();
	UTexture(const FString& name);
	virtual ~UTexture();

	virtual bool Load(const unsigned char* pixels) = 0;
	virtual bool LoadWriteable() = 0;
	virtual bool Unload() = 0;
	virtual void Destroy() = 0;

	virtual bool Resize(uint32_t new_width, uint32_t new_height) = 0;
	virtual bool WriteTextureData(uint64_t size, const unsigned char* pixels) = 0;
	virtual TArray<uint8_t> ReadTextureData(uint32_t offset, uint32_t size) =0;
	virtual FColor ReadTexturePixel(uint32_t x, uint32_t y) =0;

public:
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

	void SetFlag(TextureFlag f) { Flags = f; }
	void AddFlag(TextureFlag f) { Flags |= f; }
	TextureFlag GetFlags() const { return Flags; }
	void RemoveFlag(TextureFlag f) { Flags &= ~f; }

	void SetGeneration(uint32_t g) { Generation = g; }
	uint32_t GetGeneration() const { return Generation; }

protected:
	// TODO: Remove to UAsset
	size_t ReferenceCount;
	bool AutoRelease;
	// ------------------------

	TextureType Type = TextureType::eTexture_Type_2D;
	uint32_t Width = 100;
	uint32_t Height = 100;
	uint32_t ChannelCount = 4;

	/** TextureFlag */
	TextureFlag Flags = 0;

	uint32_t Generation = INVALID_ID;

	unsigned char* Pixels = nullptr;
};
