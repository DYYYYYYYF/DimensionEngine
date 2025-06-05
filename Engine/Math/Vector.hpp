#pragma once
#include "DMath.hpp"
#include "Core/DMemory.hpp"
#include "SIMD/SIMDHelper.hpp"

#if IS_CPP17_OR_HIGHER  
#define CONSTEXPR_IF if constexpr
#else
#define CONSTEXPR_IF if
#endif

template<typename T>
struct alignas(8) TVector2 {
	static_assert(std::is_floating_point<T>::value);

public:
	union {
		T elements[2] = { T(0) };
		struct {
			union { T x, r, s, u; };
			union { T y, g, t, v; };
		};
	};

public:
	// 添加constexpr构造函数
	constexpr TVector2() noexcept : elements{ T(0), T(0) } {}
	constexpr TVector2(T x, T y) noexcept : x(x), y(y) {}
	constexpr TVector2(const TVector2& v) noexcept = default;

	// 添加移动语义
	constexpr TVector2(TVector2&& v) noexcept = default;
	TVector2& operator=(const TVector2& v) noexcept = default;
	TVector2& operator=(TVector2&& v) noexcept = default;

	constexpr void Zero() noexcept { x = T(0); y = T(0); }
	constexpr void One() noexcept { x = T(1); y = T(1); }

	constexpr T LengthSquared() const noexcept { return x * x + y * y; }
	T Length() const noexcept { return Dsqrt(LengthSquared()); }

	// 性能优化：区分修改和非修改版本
	TVector2& NormalizeInPlace() noexcept {
		T l = Length();
		if (l < std::numeric_limits<T>::epsilon()) {
			Zero();
			return *this;
		}
		x /= l; y /= l;
		return *this;
	}

	TVector2 Normalized() const noexcept {
		TVector2 result(*this);
		return result.NormalizeInPlace();
	}

	constexpr bool Compare(const TVector2& vec, T tolerance = T(0.000001)) const noexcept {
		return Dabs(x - vec.x) <= tolerance && Dabs(y - vec.y) <= tolerance;
	}

	constexpr T Distance(const TVector2& vec) const noexcept {
		TVector2 d{ x - vec.x, y - vec.y };
		return d.Length();
	}

	// 性能优化：操作符添加const和noexcept
	constexpr TVector2 operator+(const TVector2& v) const noexcept {
		return TVector2{ x + v.x, y + v.y };
	}

	constexpr TVector2 operator-(const TVector2& v) const noexcept {
		return TVector2{ x - v.x, y - v.y };
	}

	constexpr TVector2 operator*(T num) const noexcept {
		return TVector2{ x * num, y * num };
	}

	constexpr TVector2 operator/(T num) const noexcept {
		return TVector2{ x / num, y / num };
	}

	constexpr TVector2 operator-() const noexcept {
		return TVector2(-x, -y);
	}

	constexpr bool operator==(const TVector2& other) const noexcept {
		return Compare(other);
	}

	constexpr bool operator!=(const TVector2& other) const noexcept {
		return !(*this == other);
	}
};

template<typename T>
struct TVector3 {
	static_assert(std::is_floating_point<T>::value);

public:
	union {
		T elements[3] = { T(0) };
		struct {
			union { T x, r, s, u; };
			union { T y, g, t, v; };
			union { T z, b, p, w; };
		};
	};

public:
	// 修复构造函数中的变量命名错误
	constexpr TVector3() noexcept : elements{ T(0), T(0), T(0) } {}
	constexpr TVector3(T xyz) noexcept : x(xyz), y(xyz), z(xyz) {}
	constexpr TVector3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}  // 修复：使用正确的变量名
	constexpr TVector3(const TVector3& v) noexcept = default;

	// 添加移动语义
	constexpr TVector3(TVector3&& v) noexcept = default;
	TVector3& operator=(const TVector3& v) noexcept = default;
	TVector3& operator=(TVector3&& v) noexcept = default;

	constexpr void Zero() noexcept { x = T(0); y = T(0); z = T(0); }
	constexpr void One() noexcept { x = T(1); y = T(1); z = T(1); }

	constexpr T LengthSquared() const noexcept { return x * x + y * y + z * z; }
	T Length() const noexcept { return Dsqrt(LengthSquared()); }

	// 性能优化：区分修改和非修改版本，移除重复的Normalize函数
	TVector3& NormalizeInPlace() noexcept {
		T l = Length();
		if (l < std::numeric_limits<T>::epsilon()) {
			Zero();
			return *this;
		}
		x /= l; y /= l; z /= l;
		return *this;
	}

	TVector3 Normalized() const noexcept {
		TVector3 result(*this);
		return result.NormalizeInPlace();
	}

	constexpr bool Compare(const TVector3& vec, T tolerance = T(0.000001)) const noexcept {
		return Dabs(x - vec.x) <= tolerance &&
			Dabs(y - vec.y) <= tolerance &&
			Dabs(z - vec.z) <= tolerance;
	}

	constexpr T Dot(const TVector3& vec) const noexcept {
		return x * vec.x + y * vec.y + z * vec.z;
	}

	constexpr TVector3 Cross(const TVector3& vec) const noexcept {
		return TVector3{
			y * vec.z - z * vec.y,
			z * vec.x - x * vec.z,
			x * vec.y - y * vec.x
		};
	}

	// 修复返回类型不一致问题
	T Distance(const TVector3& vec) const noexcept {
		TVector3 d{ x - vec.x, y - vec.y, z - vec.z };
		return d.Length();
	}

	TVector3 Transform(const struct TMatrix4<T>& m) const noexcept {
		return TVector3{
			x * m.data[0] + y * m.data[4] + z * m.data[8] + m.data[12],
			x * m.data[1] + y * m.data[5] + z * m.data[9] + m.data[13],
			x * m.data[2] + y * m.data[6] + z * m.data[10] + m.data[14]
		};
	}

	static constexpr TVector3 Forward() noexcept { return TVector3(T(0), T(0), T(-1)); }
	static constexpr TVector3 Backward() noexcept { return TVector3(T(0), T(0), T(1)); }
	static constexpr TVector3 Left() noexcept { return TVector3(T(-1), T(0), T(0)); }
	static constexpr TVector3 Right() noexcept { return TVector3(T(1), T(0), T(0)); }
	static constexpr TVector3 Up() noexcept { return TVector3(T(0), T(1), T(0)); }
	static constexpr TVector3 Down() noexcept { return TVector3(T(0), T(-1), T(0)); }

	constexpr TVector3 operator+(const TVector3& v) const noexcept {
		return TVector3{ x + v.x, y + v.y, z + v.z };
	}

	TVector3 operator+(TVector3&& v) const noexcept {
		v.x += x; v.y += y; v.z += z;
		return std::move(v);
	}

	constexpr TVector3 operator-(const TVector3& v) const noexcept {
		return TVector3{ x - v.x, y - v.y, z - v.z };
	}

	TVector3 operator-(TVector3&& v) const noexcept {
		v.x = x - v.x; v.y = y - v.y; v.z = z - v.z;
		return std::move(v);
	}

	constexpr TVector3 operator*(const TVector3& v) const noexcept {
		return TVector3{ x * v.x, y * v.y, z * v.z };
	}

	constexpr TVector3 operator*(T num) const noexcept {
		return TVector3{ x * num, y * num, z * num };
	}

	constexpr TVector3 operator/(const TVector3& v) const noexcept {
		return TVector3{ x / v.x, y / v.y, z / v.z };
	}

	constexpr TVector3 operator/(T num) const noexcept {
		return TVector3{ x / num, y / num, z / num };
	}

	constexpr TVector3 operator-() const noexcept {
		return TVector3(-x, -y, -z);
	}

	TVector3& operator+=(const TVector3& v) noexcept {
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	TVector3& operator-=(const TVector3& v) noexcept {
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	TVector3& operator*=(T scalar) noexcept {
		x *= scalar; y *= scalar; z *= scalar;
		return *this;
	}

	TVector3& operator/=(T scalar) noexcept {
		x /= scalar; y /= scalar; z /= scalar;
		return *this;
	}
};

template<typename T>
struct alignas(16) TVector4 {  // 32字节对齐支持AVX
	static_assert(std::is_floating_point<T>::value);

public:
	union {
#if defined(SIMD_SUPPORTED)
		// 根据类型选择合适的SIMD类型
		typename std::conditional_t<
			std::is_same_v<T, float>,
			__m128,      // float: 128位，4个float
			__m256d      // double: 256位，4个double
		> data;
#endif
		T elements[4] = { T(0) };
		struct {
			union { T x, r, s; };
			union { T y, g, t; };
			union { T z, b, p; };
			union { T w, a, q; };
		};
	};

private:
	// 使用模板特化的SIMD辅助函数
	void load_simd() noexcept {
#if defined(SIMD_SUPPORTED)
		SIMDHelper<T>::load(elements, data);
#endif
	}

	void store_simd() noexcept {
#if defined(SIMD_SUPPORTED)
		SIMDHelper<T>::store(elements, data);
#endif
	}

public:
	// 构造函数
	TVector4() noexcept : elements{ T(0), T(0), T(0), T(0) } {
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	TVector4(const TVector3<T>& vec, T w = T(1)) noexcept : x(vec.x), y(vec.y), z(vec.z), w(w) {
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	TVector4(T xyzw) noexcept : x(xyzw), y(xyzw), z(xyzw), w(xyzw) {
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	TVector4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	TVector4(const TVector4& v) noexcept = default;
	TVector4(TVector4&& v) noexcept = default;
	TVector4& operator=(const TVector4& v) noexcept = default;
	TVector4& operator=(TVector4&& v) noexcept = default;

	void Zero() noexcept {
		x = y = z = w = T(0);
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	void One() noexcept {
		x = y = z = w = T(1);
#if defined(SIMD_SUPPORTED)
		load_simd();
#endif
	}

	// LengthSquared - 使用SIMDHelper
	T LengthSquared() const noexcept {
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			auto squared = SIMDHelper<T>::mul(data, data);
			return SIMDHelper<T>::horizontal_add(squared);
		}
#endif
		return x * x + y * y + z * z + w * w;
	}

	T Length() const noexcept { return Dsqrt(LengthSquared()); }

	// Normalize - 使用SIMDHelper
	TVector4& NormalizeInPlace() noexcept {
		T len = Length();
		if (len < std::numeric_limits<T>::epsilon()) {
			Zero();
			return *this;
		}

#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			auto len_vec = SIMDHelper<T>::set1(len);
			data = SIMDHelper<T>::div(data, len_vec);
			store_simd();
		}
		else {
#endif
			x /= len; y /= len; z /= len; w /= len;
#if defined(SIMD_SUPPORTED)
		}
#endif
		return *this;
	}

	TVector4 Normalized() const noexcept {
		TVector4 result(*this);
		return result.NormalizeInPlace();
	}

	constexpr bool Compare(const TVector4& vec, T tolerance = T(0.000001)) const noexcept {
		return Dabs(x - vec.x) <= tolerance && Dabs(y - vec.y) <= tolerance &&
			Dabs(z - vec.z) <= tolerance && Dabs(w - vec.w) <= tolerance;
	}

	// Dot product - 使用SIMDHelper
	T Dot(const TVector4& vec) const noexcept {
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			auto product = SIMDHelper<T>::mul(data, vec.data);
			return SIMDHelper<T>::horizontal_add(product);
		}
#endif
		return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
	}

	T Distance(const TVector4& vec) const noexcept {
		TVector4 diff = *this - vec;
		return diff.Length();
	}

	static TVector4 StringToVec4(const char* str) {
		if (str == nullptr) {
			return TVector4{ T(1), T(1), T(1), T(1) };
		}

		TVector4 result;
		CONSTEXPR_IF(std::is_same<T, float>::value) {
			sscanf(str, "%f %f %f %f", &result.x, &result.y, &result.z, &result.w);
		}
		else CONSTEXPR_IF(std::is_same<T, double>::value) {
			sscanf(str, "%lf %lf %lf %lf", &result.x, &result.y, &result.z, &result.w);
		}
#if defined(SIMD_SUPPORTED)
		result.load_simd();
#endif
		return result;
	}

	static TVector4 Identity() noexcept {
		return TVector4(T(0), T(0), T(0), T(1));
	}

	// 算术操作符 - 使用SIMDHelper
	TVector4 operator+(const TVector4& vec) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			result.data = SIMDHelper<T>::add(data, vec.data);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x + vec.x, y + vec.y, z + vec.z, w + vec.w };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator-(const TVector4& vec) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			result.data = SIMDHelper<T>::sub(data, vec.data);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x - vec.x, y - vec.y, z - vec.z, w - vec.w };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator*(const TVector4& vec) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			result.data = SIMDHelper<T>::mul(data, vec.data);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x * vec.x, y * vec.y, z * vec.z, w * vec.w };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator*(T num) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			auto scalar_vec = SIMDHelper<T>::set1(num);
			result.data = SIMDHelper<T>::mul(data, scalar_vec);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x * num, y * num, z * num, w * num };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator/(T num) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			auto scalar_vec = SIMDHelper<T>::set1(num);
			result.data = SIMDHelper<T>::safe_div(data, scalar_vec);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x / num, y / num, z / num, w / num };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator/(const TVector4& vec) const noexcept {
		TVector4 result;
#if defined(SIMD_SUPPORTED)
		CONSTEXPR_IF(std::is_same_v<T, float> || std::is_same_v<T, double>) {
			result.data = SIMDHelper<T>::safe_div(data, vec.data);
			result.store_simd();
		}
 else {
#endif
			result = TVector4{ x / vec.x, y / vec.y, z / vec.z, w / vec.w };
#if defined(SIMD_SUPPORTED)
			}
#endif
			return result;
	}

	TVector4 operator-() const noexcept {
		return TVector4(-x, -y, -z, -w);
	}

	// 复合赋值操作符
	TVector4& operator+=(const TVector4& vec) noexcept {
		*this = *this + vec;
		return *this;
	}

	TVector4& operator-=(const TVector4& vec) noexcept {
		*this = *this - vec;
		return *this;
	}

	TVector4& operator*=(T scalar) noexcept {
		*this = *this * scalar;
		return *this;
	}

	TVector4& operator/=(T scalar) noexcept {
		*this = *this / scalar;
		return *this;
	}

	friend std::ostream& operator<<(std::ostream& os, const TVector4& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " w: " << vec.w << "\n";
	}
};