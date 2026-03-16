#include "Texture.hpp"

UTexture::UTexture() : UAsset() {
	ID = INVALID_ID;
	ReferenceCount = 0;
	AutoRelease = false;
	AssetType = EAssetType::Texture;
	Type = TextureType::eTexture_Type_2D;
	Width = 100;
	Height = 100;
	ChannelCount = 4;
	Flags = 0;
	Generation = INVALID_ID;
}

UTexture::UTexture(const FString& name) : UAsset(name) {
	ID = INVALID_ID;
	ReferenceCount = 0;
	AutoRelease = false;
	AssetType = EAssetType::Texture;
	Type = TextureType::eTexture_Type_2D;
	Width = 100;
	Height = 100;
	ChannelCount = 4;
	Flags = 0;
	Generation = INVALID_ID;
}

UTexture::~UTexture() {}