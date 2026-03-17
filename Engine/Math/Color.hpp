#pragma once

#include "Containers/TArray.hpp"

struct FColor {
	FColor() = default;
	FColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : R(r), G(g), B(b), A(a) {}
	FColor(const TArray<uint8_t>& arr) {
		if (arr.Size() != 4) {
			GLOG(Log::eError, "Invalid array size for FColor constructor. Expected 4, got %zu.", arr.Size());
			return;
		}

		R = arr[0];
		G = arr[1];
		B = arr[2];
		A = arr[3];
	}

	uint8_t R = 0;
	uint8_t G = 0;
	uint8_t B = 0;
	uint8_t A = 255;

	TArray<uint8_t> ToArray() const {
		return { R, G, B, A };
	}

	// 和 FLinearColor 互转
	struct FLinearColor ToLinear() const;
};

struct FLinearColor {
	float R = 0.f;
	float G = 0.f;
	float B = 0.f;
	float A = 1.f;

	FColor ToColor() const {
		return {
			static_cast<uint8_t>(R * 255.f),
			static_cast<uint8_t>(G * 255.f),
			static_cast<uint8_t>(B * 255.f),
			static_cast<uint8_t>(A * 255.f)
		};
	}
};

inline FLinearColor FColor::ToLinear() const {
	return {
		R / 255.f,
		G / 255.f,
		B / 255.f,
		A / 255.f
	};
}