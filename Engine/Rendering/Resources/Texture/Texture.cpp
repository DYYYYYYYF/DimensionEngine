#include "Texture.hpp"

UTexture::UTexture() : UAsset() {
	ReferenceCount = 0;
	AutoRelease = false;
	AssetType = EAssetType::Texture;
	Type = TextureType::eTexture_Type_2D;
	Width = 100;
	Height = 100;
	ChannelCount = 4;
	Flags = 0;
}

UTexture::UTexture(const FString& name) : UAsset(name) {
	ReferenceCount = 0;
	AutoRelease = false;
	AssetType = EAssetType::Texture;
	Type = TextureType::eTexture_Type_2D;
	Width = 100;
	Height = 100;
	ChannelCount = 4;
	Flags = 0;
}

UTexture::~UTexture() {}