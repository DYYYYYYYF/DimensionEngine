#include <Math/MathTypes.hpp>

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

	Vector4d v1(1.0, 2.0, 3.0, 4.0);
	Vector4d v2(5.0, 6.0, 7.0, 8.0);

	GLOG(Log::eInfo, "Add:");
	Vector4d v3 = v1 + v2;
	std::cout << v3 << std::endl;

	GLOG(Log::eInfo, "Mul:");
	Vector4d v4 = v1 * 2.0;
	std::cout << v4 << std::endl;

}
