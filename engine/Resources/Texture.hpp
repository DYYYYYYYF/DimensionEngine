#pragma once

#include "Math/MathTypes.hpp"

class Texture {
public:
	Texture() {}
	virtual ~Texture() {}

public:
	uint32_t Id;
	uint32_t Width;
	uint32_t Height;

	int ChannelCount;
	bool HasTransparency;

	uint32_t Generation;
	void* InternalData;
};