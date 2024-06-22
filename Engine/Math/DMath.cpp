#include "DMath.hpp"
#include "MathTypes.hpp"

#include <math.h>
#include <random>

// Ref: 作者：快乐大鸭脖 https://www.bilibili.com/read/cv31060791/
static std::random_device m_seed;

#if DPLATFORM_WINDOWS
static std::mt19937 m_mt19937 = std::mt19937(m_seed());
#else
static std::mt19937 m_mt19937;
#endif

float DSin(float x) {
	return sinf(x);
}

float DCos(float x) {
	return cosf(x);
}

float DTan(float x) {
	return tanf(x);
}

float DAcos(float x) {
	return acosf(x);
}

float Dabs(float x) {
	return fabs(x);
}

float Dsqrt(float x) {
	return sqrtf(x);
}


int DRandom() {
	return DRandom(-INT_MAX, INT_MAX);
}

int DRandom(int min, int max) {
	std::uniform_int_distribution<int> distribution(min, max);
	return static_cast<int>(distribution(m_mt19937));
}

float DRandom(float min, float max) {
	std::uniform_real_distribution<float> distribution(min, max);
	return static_cast<float>(distribution(m_mt19937));
}

void RGB2Vec(unsigned int r, unsigned int g, unsigned int b, struct Vec3* rgb) {
	rgb->r = r / 255.0f;
	rgb->g = g / 255.0f;
	rgb->b = b / 255.0f;
}

void Vec2RGB(struct Vec3 rgb, unsigned int* r, unsigned int* g, unsigned int* b) {
	*r = static_cast<int>(rgb.r * 255);
	*g = static_cast<int>(rgb.g * 255);
	*b = static_cast<int>(rgb.b * 255);
}