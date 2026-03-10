#include "Texture.hpp"

Texture::Texture() {
	ID = INVALID_ID;
	ReferenceCount = 0;
	AutoRelease = false;
	Type = TextureType::eTexture_Type_2D;
	Width = 100;
	Height = 100;
	ChannelCount = 4;
	Flags = 0;
	Generation = INVALID_ID;
	InternalData = nullptr;
}

Texture::~Texture() {}