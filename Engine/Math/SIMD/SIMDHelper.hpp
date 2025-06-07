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

#ifndef SIMD_SUPPORTED
#if defined(SIMD_SUPPORTED_SSE2) || defined(SIMD_SUPPORTED_AVX2) || defined(SIMD_SUPPORTED_NEON)
#define SIMD_SUPPORTED
#endif
#endif

#if defined(SIMD_SUPPORTED)

// 包含必要的头文件
#if defined(SIMD_SUPPORTED_SSE2) || defined(SIMD_SUPPORTED_AVX2)
    #include <immintrin.h>
    #include <smmintrin.h>
#endif

// SSE2/AVX2 实现 (x86/x64)
#if defined(SIMD_SUPPORTED_SSE2) || defined(SIMD_SUPPORTED_AVX2)

template<>
class SIMDHelper<float> {
public:
    using SIMDType = __m128;

public:
    static void load(float* elements, __m128& data) {
        data = _mm_load_ps(elements);
    }

    static void store(float* elements, const __m128& data) {
        _mm_store_ps(elements, data);
    }

    static float horizontal_add(const __m128& v) {
#if defined(SIMD_SUPPORTED_AVX2)
        __m128 result = _mm_hadd_ps(v, v);
        result = _mm_hadd_ps(result, result);
        return _mm_cvtss_f32(result);
#else // SSE2
        __m128 temp = _mm_add_ps(v, _mm_movehl_ps(v, v));
        temp = _mm_add_ss(temp, _mm_shuffle_ps(temp, temp, 1));
        return _mm_cvtss_f32(temp);
#endif
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
#if defined(SIMD_SUPPORTED_AVX2)
        __m128 safe_denom = _mm_blendv_ps(b, _mm_set1_ps(std::numeric_limits<float>::epsilon()), mask);
#else // SSE2
        __m128 epsilon_vec = _mm_set1_ps(std::numeric_limits<float>::epsilon());
        __m128 safe_denom = _mm_or_ps(_mm_andnot_ps(mask, b), _mm_and_ps(mask, epsilon_vec));
#endif
        return _mm_div_ps(a, safe_denom);
    }
};

// double特化 (需要AVX2支持)
#if defined(SIMD_SUPPORTED_AVX2)
template<>
class SIMDHelper<double> {
public:
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
#endif // AVX2 support for double

#endif // SSE2/AVX2 implementation

// NEON 实现 (ARM)
#if defined(SIMD_SUPPORTED_NEON)

template<>
class SIMDHelper<float> {
public:
    using SIMDType = float32x4_t;

public:
    static void load(float* elements, float32x4_t& data) {
        data = vld1q_f32(elements);
    }

    static void store(float* elements, const float32x4_t& data) {
        vst1q_f32(elements, data);
    }

    static float horizontal_add(const float32x4_t& v) {
#ifdef __aarch64__
        return vaddvq_f32(v);
#else
        float32x2_t sum = vadd_f32(vget_low_f32(v), vget_high_f32(v));
        sum = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
#endif
    }

    static float32x4_t set1(float value) {
        return vdupq_n_f32(value);
    }

    static float32x4_t add(const float32x4_t& a, const float32x4_t& b) {
        return vaddq_f32(a, b);
    }

    static float32x4_t sub(const float32x4_t& a, const float32x4_t& b) {
        return vsubq_f32(a, b);
    }

    static float32x4_t mul(const float32x4_t& a, const float32x4_t& b) {
        return vmulq_f32(a, b);
    }

    static float32x4_t div(const float32x4_t& a, const float32x4_t& b) {
#ifdef __aarch64__
        return vdivq_f32(a, b);
#else
        // ARM32 NEON 没有直接除法，使用倒数估算
        float32x4_t reciprocal = vrecpeq_f32(b);
        // 牛顿-拉夫逊迭代提高精度
        reciprocal = vmulq_f32(vrecpsq_f32(b, reciprocal), reciprocal);
        return vmulq_f32(a, reciprocal);
#endif
    }

    static float32x4_t safe_div(const float32x4_t& a, const float32x4_t& b) {
        float32x4_t zero = vdupq_n_f32(0.0f);
        uint32x4_t mask = vceqq_f32(b, zero);
        float32x4_t epsilon_vec = vdupq_n_f32(std::numeric_limits<float>::epsilon());
        float32x4_t safe_denom = vbslq_f32(mask, epsilon_vec, b);
        
#ifdef __aarch64__
        return vdivq_f32(a, safe_denom);
#else
        // ARM32 安全除法
        float32x4_t reciprocal = vrecpeq_f32(safe_denom);
        reciprocal = vmulq_f32(vrecpsq_f32(safe_denom, reciprocal), reciprocal);
        return vmulq_f32(a, reciprocal);
#endif
    }
};

// double特化 (仅 ARM64 支持)
#ifdef __aarch64__
template<>
class SIMDHelper<double> {
public:
    using SIMDType = float64x2_t;

public:
    static void load(double* elements, float64x2_t& data) {
        data = vld1q_f64(elements);
    }

    static void store(double* elements, const float64x2_t& data) {
        vst1q_f64(elements, data);
    }

    static double horizontal_add(const float64x2_t& v) {
        return vaddvq_f64(v);
    }

    static float64x2_t set1(double value) {
        return vdupq_n_f64(value);
    }

    static float64x2_t add(const float64x2_t& a, const float64x2_t& b) {
        return vaddq_f64(a, b);
    }

    static float64x2_t sub(const float64x2_t& a, const float64x2_t& b) {
        return vsubq_f64(a, b);
    }

    static float64x2_t mul(const float64x2_t& a, const float64x2_t& b) {
        return vmulq_f64(a, b);
    }

    static float64x2_t div(const float64x2_t& a, const float64x2_t& b) {
        return vdivq_f64(a, b);
    }

    static float64x2_t safe_div(const float64x2_t& a, const float64x2_t& b) {
        float64x2_t zero = vdupq_n_f64(0.0);
        uint64x2_t mask = vceqq_f64(b, zero);
        float64x2_t epsilon_vec = vdupq_n_f64(std::numeric_limits<double>::epsilon());
        float64x2_t safe_denom = vbslq_f64(mask, epsilon_vec, b);
        return vdivq_f64(a, safe_denom);
    }
};
#endif // __aarch64__

#endif // NEON implementation

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

#endif // SIMD_SUPPORTED
