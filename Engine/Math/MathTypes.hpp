#pragma once

#include "Defines.hpp"
#include "DMath.hpp"

#include "Vector.hpp"
#include "Vertex.hpp"
#include "Matrix.hpp"
#include "Frustum.hpp"
#include "Quaternion.hpp"

#include "ForwardDeclarations.hpp"

struct DAPI Axis {
	inline static TVector3<float> X = TVector3<float>{ 1.0f, 0.0f, 0.0f };
	inline static TVector3<float> Y = TVector3<float>{ 0.0f, 1.0f, 0.0f };
	inline static TVector3<float> Z = TVector3<float>{ 0.0f, 0.0f, 1.0f };
};

template<typename T>
inline T RangeConvertfloat(T value, T old_min, T old_max, T new_min, T new_max) {
	return (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

inline void RGB2Uint(unsigned int r, unsigned int g, unsigned int b, unsigned int* rgb) {
	*rgb = (((r & 0x0FF) << 16) | ((g & 0x0FF) << 8) | (b & 0xFF));
}

inline void UInt2RGB(unsigned int rgb, unsigned int* r, unsigned int* g, unsigned int* b) {
	*r = (rgb >> 16) & 0x0FF;
	*g = (rgb >> 8) & 0x0FF;
	*b = (rgb) & 0x0FF;
}

inline void UInt2RGB(uint64_t rgb, uint32_t* r, uint32_t* g, uint32_t* b) {
	*r = (rgb >> 16) & 0xFF;
	*g = (rgb >> 8) & 0xFF;
	*b = (rgb) & 0xFF;
}

inline void RGB2Vec(unsigned int r, unsigned int g, unsigned int b, TVector3<float>* rgb) {
	rgb->r = r / 255.0f;
	rgb->g = g / 255.0f;
	rgb->b = b / 255.0f;
}

inline void Vec2RGB(TVector3<float>* rgb, unsigned int* r, unsigned int* g, unsigned int* b) {
	*r = static_cast<int>(rgb->r * 255);
	*g = static_cast<int>(rgb->g * 255);
	*b = static_cast<int>(rgb->b * 255);
}

template<typename Type>
inline TQuaternion<Type> MatrixToQuat(const TMatrix4<Type>& M) {
	float trace = M[0] + M[5] + M[10];
	TQuaternion<Type> q;

	if (trace > 0) {
		float S = Dsqrt(trace + 1.0f) * 2;
		q.w = 0.25f * S;
		q.x = (M[9] - M[6]) / S;
		q.y = (M[2] - M[8]) / S;
		q.z = (M[4] - M[1]) / S;
	}
	else {
		if (M[0] > M[5] && M[0] > M[10]) {
			float S = Dsqrt(1.0f + M[0] - M[5] - M[10]) * 2;
			q.w = (M[9] - M[6]) / S;
			q.x = 0.25f * S;	
			q.y = (M[1] + M[4]) / S;
			q.z = (M[2] + M[8]) / S;
		}
		else if (M[5] > M[10]) {
			float S = Dsqrt(1.0f + M[5] - M[0] - M[10]) * 2;
			q.w = (M[2] - M[8]) / S;
			q.x = (M[1] + M[4]) / S;
			q.y = 0.25f * S;
			q.z = (M[6] + M[9]) / S;
		}
		else {
			float S = Dsqrt(1.0f + M[10] - M[0] - M[5]) * 2;
			q.w = (M[4] - M[1]) / S;
			q.x = (M[2] + M[8]) / S;
			q.y = (M[6] + M[9]) / S;
			q.z = 0.25f * S;
		}
	}

	return q;
}
