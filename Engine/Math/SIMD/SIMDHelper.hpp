#pragma once
#include "Defines.hpp"

template<typename T>
class SIMDHelper {
public:
	static void load(T* elements, int& dummy) {
		// 非SIMD版本不做任何事
	}
	static void store(T* elements, const int& dummy) {
		// 非SIMD版本不做任何事
	}
};

#if defined(SIMD_SUPPORTED)
template<>
class SIMDHelper<float> {
	using SIMDType = __m128;

public:
	static void load(float* elements, __m128& data) {
		data = _mm_load_ps(elements);
	}

	static void store(float* elements, const __m128& data) {
		_mm_store_ps(elements, data);
	}

	static float horizontal_add(const __m128& v) {
		__m128 result = _mm_hadd_ps(v, v);
		result = _mm_hadd_ps(result, result);
		return _mm_cvtss_f32(result);
	}

	static __m128 set1(float value) {
		return _mm_set1_ps(value);
	}

	static __m128 add(const __m128& a, const __m128& b) {
		return _mm_add_ps(a, b);
	}

	static __m128 sub(const __m128& a, const __m128& b) {
		return _mm_sub_ps(a, b);
	}

	static __m128 mul(const __m128& a, const __m128& b) {
		return _mm_mul_ps(a, b);
	}

	static __m128 div(const __m128& a, const __m128& b) {
		return _mm_div_ps(a, b);
	}

	static __m128 safe_div(const __m128& a, const __m128& b) {
		__m128 mask = _mm_cmpeq_ps(b, _mm_setzero_ps());
		__m128 safe_denom = _mm_blendv_ps(b, _mm_set1_ps(std::numeric_limits<float>::epsilon()), mask);
		return _mm_div_ps(a, safe_denom);
	}
};

// double特化
template<>
class SIMDHelper<double> {
	using SIMDType = __m256d;

public:
	static void load(double* elements, __m256d& data) {
		data = _mm256_load_pd(elements);
	}

	static void store(double* elements, const __m256d& data) {
		_mm256_store_pd(elements, data);
	}

	static double horizontal_add(const __m256d& v) {
		__m128d low = _mm256_castpd256_pd128(v);
		__m128d high = _mm256_extractf128_pd(v, 1);
		__m128d sum = _mm_add_pd(low, high);
		sum = _mm_hadd_pd(sum, sum);
		return _mm_cvtsd_f64(sum);
	}

	static __m256d set1(double value) {
		return _mm256_set1_pd(value);
	}

	static __m256d add(const __m256d& a, const __m256d& b) {
		return _mm256_add_pd(a, b);
	}

	static __m256d sub(const __m256d& a, const __m256d& b) {
		return _mm256_sub_pd(a, b);
	}

	static __m256d mul(const __m256d& a, const __m256d& b) {
		return _mm256_mul_pd(a, b);
	}

	static __m256d div(const __m256d& a, const __m256d& b) {
		return _mm256_div_pd(a, b);
	}

	static __m256d safe_div(const __m256d& a, const __m256d& b) {
		__m256d zero = _mm256_setzero_pd();
		__m256d mask = _mm256_cmp_pd(b, zero, _CMP_EQ_OQ);
		__m256d epsilon_vec = _mm256_set1_pd(std::numeric_limits<double>::epsilon());
		__m256d safe_denom = _mm256_blendv_pd(b, epsilon_vec, mask);
		return _mm256_div_pd(a, safe_denom);
	}
};

template<typename T>
class Vector4Batch {
public:
	static void BatchNormalize(TVector4<T>* vectors, size_t count) noexcept {
		for (size_t i = 0; i < count; ++i) {
			vectors[i].NormalizeInPlace();
		}
	}

	static void BatchScale(TVector4<T>* vectors, size_t count, T scale) noexcept {
		for (size_t i = 0; i < count; ++i) {
			vectors[i] *= scale;
		}
	}

	static void BatchAdd(TVector4<T>* result, const TVector4<T>* a, const TVector4<T>* b, size_t count) noexcept {
		for (size_t i = 0; i < count; ++i) {
			result[i] = a[i] + b[i];
		}
	}

	static void BatchDot(T* results, const TVector4<T>* a, const TVector4<T>* b, size_t count) noexcept {
		for (size_t i = 0; i < count; ++i) {
			results[i] = a[i].Dot(b[i]);
		}
	}
};
#endif
