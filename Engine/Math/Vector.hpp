#pragma once
#include "DMath.hpp"
#include "Core/DMemory.hpp"

template<typename T>
struct TVector2 {
	static_assert(std::is_floating_point<T>::value);

public:
	union
	{
		T elements[2] = { 0.0f };
		struct {
			union
			{
				T x, r, s, u;
			};
			union
			{
				T y, g, t, v;
			};
		};
	};

public:
	TVector2() { Zero(); }
	TVector2(T x, T y) {
		r = x;
		g = y;
	}

	TVector2(const TVector2& v) {
		r = v.x;
		g = v.y;
	}

	void Zero() {
		x = 0.0f;
		y = 0.0f;
	}

	void One() {
		x = 1.0f;
		y = 1.0f;
	}

	/*
	* @brief Returns the squared length of the provided vector.
	*
	* @param vector The vector to retrieve the squared length of.
	* @return The squared length.
	*/
	T LengthSquared() const { return x * x + y * y; }

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	T Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	TVector2 Normalize() {
		x /= Length();
		y /= Length();

		return *this;
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	*
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const TVector2& vec, double tolerance = 0.000001) const {
		if (Dabs(x - vec.x) > tolerance) {
			return false;
		}

		if (Dabs(y - vec.y) > tolerance) {
			return false;
		}

		return true;
	}

	/*
	* @brief Returns the distance between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	T Distance(const TVector2& vec) const {
		TVector2 d{ x - vec.x, y - vec.y };
		return d.Length();
	}

	TVector2 operator+(const TVector2& v) {
		return TVector2{ x + v.x, y + v.y };
	}

	TVector2 operator-(const TVector2& v) {
		return TVector2{ x - v.x, y - v.y };
	}

	TVector2 operator*(int num) {
		return TVector2{ x * num, y * num };
	}

	TVector2 operator*(float num) {
		return TVector2{ x * num, y * num };
	}

	TVector2 operator/(int num) {
		return TVector2{ x / num, y / num };
	}

	TVector2 operator/(float num) {
		return TVector2{ x / num, y / num };
	}

	// Negative
	TVector2 operator-() {
		return TVector2(-x, -y);
	}

	friend std::ostream& operator<<(std::ostream& os, const TVector2& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << "\n";
	}
};

template<typename T>
struct TVector3 {
	static_assert(std::is_floating_point<T>::value);

public:
	union {
		T elements[3] = { 0.0f };
		struct {
			union
			{
				// First element
				T x, r, s, u;
			};
			union
			{
				// Sec element
				T y, g, t, v;
			};
			union
			{
				// Third element
				T z, b, p, w;
			};
		};
	};

public:
	TVector3() { Zero(); }

	TVector3(float x) {
		r = x;
		g = x;
		b = x;
	}

	TVector3(float x, float y, float z) {
		r = x;
		g = y;
		b = z;
	}

	TVector3(const TVector3& v) {
		r = v.x;
		g = v.y;
		b = v.z;
	}

	void Zero() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	void One() {
		x = 1.0f;
		y = 1.0f;
		z = 1.0f;
	}

	/*
	* @brief Returns the squared length of the provided vector.
	*
	* @param vector The vector to retrieve the squared length of.
	* @return The squared length.
	*/
	T LengthSquared() const { return x * x + y * y + z * z; }

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	T Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	TVector3 Normalize() {
		T l = Length();
		if (l < FLT_MIN) {
			return 0.0f;
		}

		x = x / l;
		y = y / l;
		z = z / l;

		return *this;
	}

	TVector3 Normalize() const {
		T l = Length();
		if (l < FLT_MIN) {
			return TVector3(0.0f);
		}

		return TVector3{x / l, y / l, z / l};
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	*
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const TVector3& vec, double tolerance = 0.000001) const {
		if (Dabs(x - vec.x) > tolerance) {
			return false;
		}

		if (Dabs(y - vec.y) > tolerance) {
			return false;
		}

		if (Dabs(z - vec.z) > tolerance) {
			return false;
		}

		return true;
	}

	/*
	* @brief Returns the product between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	T Dot(const TVector3& vec) const {
		return x * vec.x + y * vec.y + z * vec.z;
	}

	/*
	* @brief Calculates and returns the cross product of two vectors.
	*
	* @param vec Another vector.
	* @return The cross product result of this vector and the other.
	*/
	TVector3 Cross(const TVector3& vec) const {
		return TVector3{
			y * vec.z - z * vec.y,
			z * vec.x - x * vec.z,
			x * vec.y - y * vec.x
		};
	}

	/*
	* @brief Returns the distance between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	float Distance(const TVector3& vec) const {
		TVector3 d{ x - vec.x, y - vec.y, z - vec.z };
		return d.Length();
	}

	/**
	 * @brief Transform v by m. NOTE: It is assumed by this function that the vector
	 * v is a point, not a direction, and is calculated as if a w component with a value of 1.0f is there.
	 *
	 * @param m The matrix to transform by.
	 * @return A transformed copy of v.
	 */
	TVector3 Transform(struct TMatrix4<T> m) {
		TVector3 out;
		out.x = x * m.data[0 + 0] + y * m.data[4 + 0] + z * m.data[8 + 0] + 1.0f * m.data[12 + 0];
		out.y = x * m.data[0 + 1] + y * m.data[4 + 1] + z * m.data[8 + 1] + 1.0f * m.data[12 + 1];
		out.z = x * m.data[0 + 2] + y * m.data[4 + 2] + z * m.data[8 + 2] + 1.0f * m.data[12 + 2];
		return out;
	}

	static TVector3 Forward() {
		return TVector3(0.0f, 0.0f, -1.0f);
	}

	static TVector3 Backward() {
		return TVector3(0.0f, 0.0f, 1.0f);
	}

	static TVector3 Left() {
		return TVector3(-1.0f, 0.0f, 0.0f);
	}

	static TVector3 Right() {
		return TVector3(1.0f, 0.0f, 0.0f);
	}

	static TVector3 Up() {
		return TVector3(0.0f, 1.0f, 0.0f);
	}

	static TVector3 Down() {
		return TVector3(0.0f, -1.0f, 0.0f);
	}

	// Add
	TVector3 operator+(const TVector3& v) {
		return TVector3{ x + v.x, y + v.y, z + v.z };
	}

	friend TVector3 operator+(const TVector3& v1, const TVector3& v2) {
		return TVector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
	}

	// Sub
	TVector3 operator-(const TVector3& v) {
		return TVector3{ x - v.x, y - v.y, z - v.z };
	}

	friend TVector3 operator-(const TVector3& v1, const TVector3& v2) {
		return TVector3{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
	}

	// Mut
	TVector3 operator*(const TVector3& v) {
		TVector3 t = TVector3(x, y, z);
		return TVector3{ t.x * v.x, t.y * v.y, t.z * v.z };
	}

	friend TVector3 operator*(const TVector3& v1, const TVector3& v2) {
		return TVector3{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
	}

	TVector3 operator*(float num) {
		TVector3 t = TVector3(x, y, z);
		return TVector3{ t.x * num, t.y * num, t.z * num };
	}

	friend TVector3 operator*(const TVector3& v, float num) {
		return TVector3{ v.x * num, v.y * num, v.z * num };
	}

	TVector3 operator/(const TVector3& v) {
		return TVector3{ x / v.x, y / v.y, z / v.z };
	}

	// Div
	TVector3 operator/(int num) {
		return TVector3{ x / num, y / num, z / num };
	}

	TVector3 operator/(T num) {
		return TVector3{ x / num, y / num, z / num };
	}

	// Negative
	TVector3 operator-() {
		return TVector3(-x, -y, -z);
	}

	friend std::ostream& operator<<(std::ostream& os, const TVector3& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << "\n";
	}
};

template<typename T>
struct alignas(16) TVector4 {
	static_assert(std::is_floating_point<T>::value);
#if defined(SIMD_SUPPORTED_NEON)
    using DataType = std::conditional_t<std::is_same_v<T, float>, float32x4_t, float64x1x4_t>;
#elif defined(SIMD_SUPPORTED)
    using DataType = std::conditional_t<std::is_same_v<T, float>, __m128, __m256d>;
#endif
    
private:
	void InitSIMDData() {
#if defined(SIMD_SUPPORTED)
		if constexpr (std::is_same_v<T, float>) {
            #if defined(SIMD_SUPPORTED_NEON)
			// For float: Use _mm256_set_ps and _mm256_add_ps
            data = vld1q_f32(&elements[0]); // Load x, y, z, w into v1
            #else
            data = _mm_set_ps(w, z, y, x);
            #endif
		}
		else {
            GLOG(Log::eFatal, "Can not support 256bit SIMD yet!");
		}
#endif
	}

public:
	union
	{
#if defined(SIMD_SUPPORTED)
		// Used for SIMD operations
		DataType data;
#endif
		// An array of x, y, z, w
		T elements[4] = { 0.0f };
		struct
		{
			union
			{
				T x, r, s;
			};
			union
			{
				T y, g, t;
			};
			union
			{
				T z, b, p;
			};
			union
			{
				T w, a, q;
			};
		};
	};

public:
	TVector4() { 
		Zero(); 
		InitSIMDData();
	}

	TVector4(TVector3<T> vec, float w = 1.0f) {
		r = vec.x;
		g = vec.y;
		b = vec.z;
		a = w;
		InitSIMDData();
	}

	TVector4(T x) {
		r = x;
		g = x;
		b = x;
		a = x;
		InitSIMDData();
	}

	TVector4(T x, T y, T z, T w) {
		r = (T)x;
		g = (T)y;
		b = (T)z;
		a = (T)w;
		InitSIMDData();
	}

	TVector4(const TVector4& v) {
		r = v.x;
		g = v.y;
		b = v.z;
		a = v.w;
		InitSIMDData();
	}

	void Zero() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
		InitSIMDData();
	}

	void One() {
		x = 1.0f;
		y = 1.0f;
		z = 1.0f;
		w = 1.0f;
		InitSIMDData();
	}

	/*
	* @brief Returns the squared length of the provided vector.
	*
	* @param vector The vector to retrieve the squared length of.
	* @return The squared length.
	*/
	T LengthSquared() const { 
#if defined(SIMD_SUPPORTED_NEON)
        float32x4_t Result = vmulq_f32(data, data);  // Element-wise multiplication
        float32x2_t sum = vadd_f32(vget_low_f32(Result), vget_high_f32(Result));  // Horizontal add for the lower and upper halves
        sum = vpadd_f32(sum, sum);  // Final horizontal add to reduce to a single value
        return vget_lane_f32(sum, 0);  // Extract the scalar value from the result
#elif defined(SIMD_SUPPORTED)
		DataType Result = _mm_mul_ps(data, data);
		Result = _mm_hadd_ps(Result, Result);
		Result = _mm_hadd_ps(Result, Result);
		return _mm_cvtss_f32(Result);
#else
		return x * x + y * y + z * z + w * w; 
#endif
	}

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	T Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	TVector4 Normalize() {
#if defined(SIMD_SUPPORTED_NEON)
        float32x4_t divd = vdupq_n_f32(Length());  // Load scalar Length() into all elements of a NEON register
        float32x4_t result = vdivq_f32(data, divd);  // Perform element-wise division
        vst1q_f32(elements, result);  // Store the result back into memory
#elif defined(SIMD_SUPPORTED)
		DataType divd = _mm_set1_ps(Length());
		data = _mm_div_ps(data, divd);
		_mm_store_ps(elements, data);
#else
		x /= Length();
		y /= Length();
		z /= Length();
		w /= Length();
#endif
		return *this;
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	*
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const TVector4& vec, double tolerance = 0.000001f) const {
		if (Dabs(x - vec.x) > tolerance) {
			return false;
		}

		if (Dabs(y - vec.y) > tolerance) {
			return false;
		}

		if (Dabs(z - vec.z) > tolerance) {
			return false;
		}

		if (Dabs(w - vec.w) > tolerance) {
			return false;
		}

		return true;
	}

	/*
	* @brief Returns the product between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	T Dot(const TVector4& vec) const {
#if defined(SIMD_SUPPORTED)
		TVector4 res;
        if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            // Load the components of `vec` into a NEON register
            float32x4_t v = vld1q_f32(&vec.x);  // Load vec.x, vec.y, vec.z, vec.w into a NEON 128-bit vector
            // Perform element-wise multiplication with `data`
            float32x4_t result = vmulq_f32(data, v);  // Multiply data and vec element-wise
            // Store the result back into the `res.data` variable
            vst1q_f32(reinterpret_cast<float*>(&res.data), result);
#elif defined(SIMD_SUPPORTED)
            __m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);  // Load scalar a into all elements of a __m256
            res.data = _mm_mul_ps(data, v);  // Perform SIMD multiplication
            _mm_store_ps(reinterpret_cast<float*>(&res.data), res.data);
#endif
        }
		else {
			GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
			ASSERT(false);
		}
		res.x = reinterpret_cast<T*>(&res.data)[0];
		res.y = reinterpret_cast<T*>(&res.data)[1];
		res.z = reinterpret_cast<T*>(&res.data)[2];
		res.w = reinterpret_cast<T*>(&res.data)[3];
		return res;
#else
		return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
#endif
	}

	/*
	* @brief Returns the distance between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	T Distance(const TVector4& vec) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
           // Load vec.x, vec.y, vec.z, vec.w into a NEON 128-bit register
           float32x4_t v = vld1q_f32(&vec.x);  // Load values into vector
           // Perform element-wise multiplication with 'data'
           float32x4_t result = vmulq_f32(data, v);  // Multiply data with vec element-wise
           // Store the result into d.data
           vst1q_f32(reinterpret_cast<float*>(&d.data), result);  // Store back to memory
#else
			__m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);  // Load scalar a into all elements of a __m128
			d.data = _mm_mul_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
		else {
			GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
			ASSERT(false);
		}
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];
#else
		TVector4 d{ x - vec.x, y - vec.y, z - vec.z, w - vec.w };
#endif
		return d.Length();
	}

	static TVector4 StringToVec4(const char* str) {
		if (str == nullptr) {
			return TVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
		}

		TVector4 Result;
		if constexpr (std::is_same<T, float>::value) {
			sscanf(str, "%f %f %f %f", &Result.x, &Result.y, &Result.z, &Result.w);
		}
		else if constexpr (std::is_same<T, double>::value) {
			sscanf(str, "%lf %lf %lf %lf", &Result.x, &Result.y, &Result.z, &Result.w);
		}
		else {
			return Result;
		}
		return Result;
	}

	static TVector4 Identity() {
		TVector4 Result = TVector4(0.0f, 0.0f, 0.0f, 1.0f);
		return Result;
	}

public:
	// Add
	TVector4 operator+(const TVector4& vec) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            // Load vec.x, vec.y, vec.z, vec.w into a NEON 128-bit register
            float32x4_t v = vld1q_f32(&vec.x);  // Load values into vector
            // Perform element-wise addition with 'data'
            float32x4_t result = vaddq_f32(data, v);  // Add data with vec element-wise
            // Store the result into d.data
            vst1q_f32(reinterpret_cast<float*>(&d.data), result);  // Store result back into memory
#elif defined(SIMD_SUPPORTED)
			__m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);  // Load scalar a into all elements of a __m128
			d.data = _mm_add_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
		else {
			GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
			ASSERT(false);
		}
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x + vec.x, y + vec.y, z + vec.z, w + vec.w };
#endif
	}

	// Sub
	TVector4 operator-(const TVector4& vec) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            float32x4_t v = {vec.x, vec.y, vec.z, vec.w};       // Load scalar values into a NEON register
            d.data = vsubq_f32(data, v);                        // Perform SIMD subtraction
            vst1q_f32(reinterpret_cast<float*>(&d.data), d.data);  // Store the result

#else
			__m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);  // Load scalar a into all elements of a __m128
			d.data = _mm_sub_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
		else {
			GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
			ASSERT(false);
		}
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x - vec.x, y - vec.y, z - vec.z, w - vec.w };
#endif
	}

	// Multiply
	TVector4 operator*(const TVector4& vec) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            // Load vec.x, vec.y, vec.z, vec.w into a NEON 128-bit register
            float32x4_t v = vld1q_f32(&vec.x);  // Load values into vector
            // Perform element-wise multiplication with 'data'
            float32x4_t result = vmulq_f32(data, v);  // Multiply data with vec element-wise
            // Store the result into d.data
            vst1q_f32(reinterpret_cast<float*>(&d.data), result);  // Store result back into memory
#else
			__m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);
			d.data = _mm_mul_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
		else {
            GLOG(Log::eError, "Not support 256bit SIMD yet.");
            ASSERT(false);
		}
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x * vec.x, y * vec.y, z * vec.z, w * vec.w };
#endif
	}

	// Divide
	TVector4 operator*(T num) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            float32x4_t v = vdupq_n_f32(num);  // Load 'num' into all elements of a NEON register
            d.data = vmulq_f32(data, v);       // Perform SIMD multiplication
            vst1q_f32(reinterpret_cast<float*>(&d.data), d.data);  // Store result back to memory
#else
			__m128 v = _mm_set1_ps(num);
			d.data = _mm_mul_ps(data, v);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
        else {
            GLOG(Log::eError, "Can not support 256bit SIMD yet.");
            ASSERT(false);
        }
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x * num, y * num, z * num, w * num };
#endif
	}

	// Div
	TVector4 operator/(T num) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            float32x4_t v = vdupq_n_f32(num);  // Load scalar into all elements of the NEON vector
            // 检查除数是否为 0
            float32x4_t zero = vdupq_n_f32(0.0f);  // Create a vector of zero
            uint32x4_t mask = vceqq_f32(v, zero);  // Check if num is zero (mask with 1 where true)
            // If divisor is zero, we use a very small value (FLT_MIN) to avoid divide-by-zero
            float32x4_t safeDenom = vbslq_f32(mask, vdupq_n_f32(FLT_MIN), v);  // Blend to choose between num and FLT_MIN
            d.data = vdivq_f32(data, safeDenom);  // Perform SIMD division
            vst1q_f32(reinterpret_cast<float*>(&d.data), d.data);
#elif defined(SIMD_SUPPORTED)
			__m128 v = _mm_set1_ps(num);  // Load scalar a into all elements of a __m256
			// 检查除数是否为 0
			__m128 mask = _mm_cmpeq_ps(v, _mm_setzero_ps());  // 如果 denom == 0, 设置掩码
			__m128 safeDenom = _mm_blendv_ps(v, _mm_set1_ps(FLT_MIN), mask); // 如果为零，则使用一个极小的数
			d.data = _mm_div_ps(data, safeDenom);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#else
            GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
            ASSERT(false);
#endif
        }
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x / num, y / num, y / num, w / num };
#endif
	}

	TVector4 operator/(const TVector4& vec) {
#if defined(SIMD_SUPPORTED)
		TVector4 d;
		if constexpr (std::is_same_v<T, float>) {
#if defined(SIMD_SUPPORTED_NEON)
            // Load vec.x, vec.y, vec.z, vec.w into a NEON 128-bit register
            float32x4_t v = vld1q_f32(reinterpret_cast<const float*>(&vec.x));  // Ensure correct alignment
            // Check if any element of 'v' is zero (division by zero check)
            float32x4_t zero = vdupq_n_f32(0.0f);  // Create a vector of zeros
            uint32x4_t mask = vceqq_f32(v, zero);  // Create a mask where 'v' == 0
            // If zero is detected, replace with FLT_MIN
            float32x4_t safeDenom = vbslq_f32(mask, vdupq_n_f32(FLT_MIN), v);  // Blend to choose between v and FLT_MIN
            // Perform element-wise division (safe division)
            float32x4_t result = vdivq_f32(data, safeDenom);  // Divide data by safeDenom
            // Store the result into d.data
            vst1q_f32(reinterpret_cast<float*>(&d.data), result);  // Store result back into memory
#else
			__m128 v = _mm_set_ps(vec.w, vec.z, vec.y, vec.x);
			// 除数检查，为0则替换
			__m128 mask = _mm_cmpeq_ps(v, _mm_setzero_ps());
			__m128 safeDenom = _mm_blendv_ps(v, _mm_set1_ps(FLT_MIN), mask);
			d.data = _mm_div_ps(data, safeDenom);  // Perform SIMD multiplication
			_mm_store_ps(reinterpret_cast<float*>(&d.data), d.data);
#endif
        }
		else {
			GLOG(Log::eFatal, "Engine not support double type SIMD yet!");
			ASSERT(false);
		}
		d.x = reinterpret_cast<T*>(&d.data)[0];
		d.y = reinterpret_cast<T*>(&d.data)[1];
		d.z = reinterpret_cast<T*>(&d.data)[2];
		d.w = reinterpret_cast<T*>(&d.data)[3];

		return d;
#else
		return TVector4{ x / vec.x, y / vec.y, z / vec.z, w / vec.w };
#endif
	}

	// Negative
	TVector4 operator-() {
		return TVector4(-x, -y, -z, -w);
	}

	friend std::ostream& operator<<(std::ostream& os, const TVector4& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " w: " << vec.w << "\n";
	}
};
