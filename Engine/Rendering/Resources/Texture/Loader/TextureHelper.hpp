#pragma once
#include "Containers/FString.hpp"

class UTexture;

class TextureHelper {
public:
	static bool Load(const FString& name, void* params, UTexture* resource);
	static void Unload(UTexture* resource);
};