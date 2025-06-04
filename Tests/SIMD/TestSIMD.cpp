#include <Math/MathTypes.hpp>

template<typename T>
struct V4 {
public:
#if defined(SIMD_SUPPORTED_NEON)
    using DataType = std::conditional_t<std::is_same_v<T, float>, float32x4_t, float64x1x4_t>;
#elif defined(SIMD_SUPPORTED)
	using DataType = std::conditional_t<std::is_same_v<T, float>, __m128, __m256d>;
#endif
    
	alignas(16) DataType data;  // 256-bit AVX register
	alignas(16) T x, y, z, w;         // Individual elements for scalar operations

	V4() : x(0), y(0), z(0), w(0) {
		if constexpr (std::is_same_v<T, float>) {
			// For float: Use _mm256_set_ps and _mm256_add_ps
			data = _mm_set_ps(w, z, y, x);  // Load x, y, z, w into v1
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
            exit(-2);
		}
	}

	V4(T r, T g, T b, T a) : x(r), y(g), z(b), w(a) {
		if constexpr (std::is_same_v<T, float>) {
			// For float: Use _mm256_set_ps and _mm256_add_ps
			data = _mm_set_ps(w, z, y, x);  // Load x, y, z, w into v1
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
            exit(-2);
		}
	}

	// SIMD operator+ using AVX
	V4 operator+(const V4& o) {
		// Load the current object's data and the other object's data into AVX registers
		DataType v1, v2, result;
		if constexpr (std::is_same_v<T, float>) {
			// For float: Use _mm256_set_ps and _mm256_add_ps
			v1 = _mm_set_ps(w, z, y, x);  // Load x, y, z, w into v1
			v2 = _mm_set_ps(o.w, o.z, o.y, o.x);  // Load o.x, o.y, o.z, o.w into v2
			result = _mm256_add_ps(v1, v2);  // Perform SIMD addition
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
            exit(-2);
		}

		// Store the result back into a V4 object
		V4 res;
		if constexpr (std::is_same_v<T, float>) {
			// For float: Use _mm256_store_ps
			_mm_store_ps(reinterpret_cast<float*>(&res.data), result);
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
            exit(-2);
		}

		// Store the scalar values from the SIMD result
		res.x = reinterpret_cast<T*>(&res.data)[0];
		res.y = reinterpret_cast<T*>(&res.data)[1];
		res.z = reinterpret_cast<T*>(&res.data)[2];
		res.w = reinterpret_cast<T*>(&res.data)[3];

		return res;
	}

	V4 operator*(T a) {
		V4 res;
        if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            float32x4_t v = vdupq_n_f32(a);  // Load scalar a into all elements of a NEON register
            float32x4_t result = vmulq_f32(data, v);  // Perform element-wise multiplication
            vst1q_f32(reinterpret_cast<float*>(&res.data), result);  // Store the result back into memory
        }
        
        res.x = reinterpret_cast<T*>(&res.data)[0];
        res.y = reinterpret_cast<T*>(&res.data)[1];
        res.z = reinterpret_cast<T*>(&res.data)[2];
        res.w = reinterpret_cast<T*>(&res.data)[3];
#elif defined(SIMD_SUPPORTED)
			__m128 v = _mm_set1_ps(a);  // Load scalar a into all elements of a __m256
			res.data = _mm_mul_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&res.data), res.data);
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
            exit(-2);
		}
		res.x = reinterpret_cast<T*>(&res.data)[0];
		res.y = reinterpret_cast<T*>(&res.data)[1];
		res.z = reinterpret_cast<T*>(&res.data)[2];
		res.w = reinterpret_cast<T*>(&res.data)[3];
#else
        res.x = x * a;
        res.y = y * a;
        res.z = z * a;
        res.w = w * a;
#endif
		return res;
	}

	// Output the vector values (excluding the SIMD data)
	friend std::ostream& operator<<(std::ostream& os, const V4& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " w: " << vec.w;
	}
};

void CheckSupportedSIMD() {
#if defined(SIMD_SUPPORTED_NEON)
	std::cout << "arm NEON is supported.\n";
#endif
#if defined(SIMD_SUPPORTED_AVX)
	std::cout << "AVX is supported.\n";
#endif
#if defined(SIMD_SUPPORTED_AVX2)
	std::cout << "AVX2 is supported.\n";
#endif
#if defined(SIMD_SUPPORTED_SSE)
	std::cout << "SSE is supported.\n";
#endif
#if defined(SIMD_SUPPORTED_SSE2)
	std::cout << "SSE2 is supported.\n";
#endif
}

void TestSIMD(){
	GLOG(Log::eInfo, "\n SIMD:\n");
	CheckSupportedSIMD();

	V4<double> v1(1.0f, 2.0, 3.0, 4.0);
	V4<double> v2(5.0f, 6.0, 7.0, 8.0);

	GLOG(Log::eInfo, "Add:");
	V4 v3 = v1 + v2;
	std::cout << v3 << std::endl;

	GLOG(Log::eInfo, "Mul:");
	V4 v4 = v1 * 2.0;
	std::cout << v4 << std::endl;

}
