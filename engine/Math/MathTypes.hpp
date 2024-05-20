#pragma once

#include "Defines.hpp"
#include "DMath.hpp"
#include "Core/DMemory.hpp"
#include <ostream>

struct Vec2 {
public:
	union
	{
		float elements[2];
		struct {
			union
			{
				float x, r, s, u;
			};
			union
			{
				float y, g, t, v;
			};
		};
	};

public:
	Vec2() { Zero(); }
	Vec2(float x, float y) {
		r = x;
		g = y;
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
	float LengthSquared() const { return x * x + y * y; }

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	float Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	void Normalize() { 
		x /= Length();
		y /= Length();
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	* 
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const Vec2& vec, float tolerance = 0.000001f) {
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
	float Distance(const Vec2& vec) {
		Vec2 d{ x - vec.x, y - vec.y };
		return d.Length();
	}

	Vec2 operator+(const Vec2& v) {
		return Vec2{ x + v.x, y + v.y };
	}

	Vec2 operator-(const Vec2& v) {
		return Vec2{ x - v.x, y - v.y };
	}

	Vec2 operator*(int num) {
		return Vec2{ x * num, y * num };
	}

	Vec2 operator*(float num) {
		return Vec2{ x * num, y * num };
	}

	Vec2 operator/(int num) {
		return Vec2{ x / num, y / num };
	}

	Vec2 operator/(float num) {
		return Vec2{ x / num, y / num };
	}

	friend std::ostream& operator<<(std::ostream& os, const Vec2& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << "\n";
	}
};

struct Vec3 {
public:
	union{
		float elements[3];
		struct {
			union
			{
				// First element
				float x, r, s, u;
			};
			union
			{
				// Sec element
				float y, g, t, v;
			};
			union
			{
				// Third element
				float z, b, p, w;
			};
		};
	};

public:
	Vec3() { Zero(); }

	Vec3(float x) {
		r = x;
		g = x;
		b = x;
	}

	Vec3(float x, float y, float z) {
		r = x;
		g = y;
		b = z;
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
	float LengthSquared() const { return x * x + y * y + z * z; }

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	float Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	Vec3 Normalize() {
		x /= Length();
		y /= Length();
		z /= Length();

		return *this;
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	*
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const Vec3& vec, float tolerance = 0.000001f) {
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
	float Dot(const Vec3& vec) {
		return x * vec.x + y * vec.y + z * vec.z;
	}

	/*
	* @brief Calculates and returns the cross product of two vectors.
	*
	* @param vec Another vector.
	* @return The cross product result of this vector and the other.
	*/
	Vec3 Cross(const Vec3& vec) {
		return Vec3{
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
	float Distance(const Vec3& vec) {
		Vec3 d{ x - vec.x, y - vec.y, z - vec.z };
		return d.Length();
	}

	// Add
	Vec3 operator+(const Vec3& v) {
		return Vec3{ x + v.x, y + v.y, z + v.z };
	}

	// Sub
	Vec3 operator-(const Vec3& v) {
		return Vec3{ x - v.x, y - v.y, z - v.z };
	}

	// Mut
	Vec3 operator*(const Vec3& v) {
		return Vec3{ x * v.x, y * v.y, z * v.z };
	}

	Vec3 operator*(int num) {
		return Vec3{ x * num, y * num, z * num };
	}

	Vec3 operator*(float num) {
		return Vec3{ x * num, y * num, z * num };
	}

	Vec3 operator/(const Vec3& v) {
		return Vec3{ x / v.x, y / v.y, z / v.z };
	}

	// Div
	Vec3 operator/(int num) {
		return Vec3{ x / num, y / num, y / num };
	}

	Vec3 operator/(float num) {
		return Vec3{ x / num, y / num, y / num };
	}

	friend std::ostream& operator<<(std::ostream& os, const Vec3& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << "\n";
	}
};

struct Vec4 {
public:
	union
	{
#if defined(DUSE_SIMD)
		// Used for SIMD operations
		alignas(16) __m128 data;
#endif
		// An array of x, y, z, w
		alignas(16) float elements[4];
		struct
		{
			union
			{
				float x, r, s;
			};
			union
			{
				float y, g, t;
			};
			union
			{
				float z, b, p;
			};
			union
			{
				float w, a, q;
			};
		};
	};

public:
	Vec4() { Zero(); }

	Vec4(Vec3 vec, float w = 1.0f) {
#if defined(DUSE_SIMD)
		data = _mm_setr_ps(x, y, z, w);
#else
		r = vec.x;
		g = vec.y;
		b = vec.z;
		a = w;
#endif
	}

	Vec4(float x) {
		r = x;
		g = x;
		b = x;
		a = x;
	}

	Vec4(float x, float y, float z, float w) {
		r = x;
		g = y;
		b = z;
		a = w;
	}

	void Zero() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 0.0f;
	}

	void One() {
		x = 1.0f;
		y = 1.0f;
		z = 1.0f;
		w = 1.0f;
	}

	/*
	* @brief Returns the squared length of the provided vector.
	*
	* @param vector The vector to retrieve the squared length of.
	* @return The squared length.
	*/
	float LengthSquared() const { return x * x + y * y + z * z + w * w; }

	/*
	* @brief Returns the length of the provided vector.
	*
	* @param vector The vector to retrieve the length of.
	* @return The length.
	*/
	float Length() const { return Dsqrt(LengthSquared()); }

	/*
	* @brief Normalizes vector
	*/
	Vec4 Normalize() {
		x /= Length();
		y /= Length();
		z /= Length();
		w /= Length();

		return *this;
	}

	/*
	* @brief Compares all elements of vector and ensures the difference is less than tolerance.
	*
	* @param vec The other vector2.
	* @param tolerance The difference tolerance. Typically K_FLOAT_EPSILON or similar.
	* @return True if within tolerance, otherwise false.
	*/
	bool Compare(const Vec4& vec, float tolerance = 0.000001f) {
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
	float Dot(const Vec4& vec) {
		return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
	}

	/*
	* @brief Returns the distance between two vectors.
	*
	* @param vec Another vector.
	* @return The distance between this vector and the other.
	*/
	float Distance(const Vec4& vec) {
		Vec4 d{ x - vec.x, y - vec.y, z - vec.z, w - vec.w };
		return d.Length();
	}

	static Vec4 StringToVec4(const char* str) {
		if (str == nullptr) {
			return Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		}

		Vec4 Result;
		sscanf(str, "%f %f %f %f", &Result.x, &Result.y, &Result.z, &Result.w);
		return Result;
	}

	//-----------------------------------------------
	// Quaternion
	//-----------------------------------------------
	Vec4 QuaternionIdentity() {
		return Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
	}

	float QuaternionNormal() const {
		return Dsqrt(
			x * x +
			y * y +
			z * z +
			w * w
		);
	}

	Vec4 QuaternionConjugate() {
		x *= -1;
		y *= -1;
		z *= -1;
		return *this;
	}

	Vec4 QuaternionInverse() {
		return QuaternionConjugate().Normalize();
	}

	Vec4 QuaternionMultiply(const Vec4& q) {
		x = x * q.w +
			y * q.z -
			z * q.y +
			w * q.x;
		y = -x * q.z +
			y * q.w +
			z * q.x +
			w * q.y;
		z = x * q.y -
			y * q.x +
			z * q.w +
			w * q.z;
		w = -x * q.x -
			y * q.y -
			z * q.z +
			w * q.w;

		return *this;
	}

	float QuaternionDot(const Vec4& v) {
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	static Vec4 Identity() {
		Vec4 Result = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		return Result;
	}

public:
	// Add
	Vec4 operator+(const Vec4& v) {
		return Vec4{ x + v.x, y + v.y, z + v.z, w + v.w };
	}

	// Sub
	Vec4 operator-(const Vec4& v) {
		return Vec4{ x - v.x, y - v.y, z - v.z, w - v.w };
	}

	// Mut
	Vec4 operator*(const Vec4& v) {
		return Vec4{ x * v.x, y * v.y, z * v.z, w * v.w };
	}

	Vec4 operator*(int num) {
		return Vec4{ x * num, y * num, z * num, w * num };
	}

	Vec4 operator*(float num) {
		return Vec4{ x * num, y * num, z * num, w * num };
	}

	// Div
	Vec4 operator/(int num) {
		return Vec4{ x / num, y / num, y / num, w / num };
	}

	Vec4 operator/(float num) {
		return Vec4{ x / num, y / num, y / num, w / num };
	}

	Vec4 operator/(const Vec4& v) {
		return Vec4{ x / v.x, y / v.y, z / v.z, w / v.w };
	}

	friend std::ostream& operator<<(std::ostream& os, const Vec4& vec) {
		return os << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << " w: " << vec.w << "\n";
	}
};

typedef Vec4 Quaternion;

inline Vec3 ToVec3(const Vec4& vec) {
	return Vec3{ vec.x, vec.y, vec.z };
}

inline Vec4 ToVec4(const Vec3& vec, float w) {
	return Vec4{ vec, w };
}

struct Matrix4 {
public:
	union {
		alignas(16) float data[16];
	};

	Matrix4() {
		Memory::Zero(data, sizeof(float) * 16);
	}

	/*
	* @brief Returns the result of multiplying 
	* 
	* @param mat The matrix to be multiplied.
	* @return The result of the matrix multiplication.
	*/
	Matrix4 Multiply(const Matrix4& mat) {
		const float* MatPtr1 = data;
		const float* MatPtr2 = mat.data;

		Matrix4 NewMat = Matrix4::Identity();
		float* DstPtr = NewMat.data;

		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; j++) {
				*DstPtr = MatPtr1[0] * MatPtr2[0 + j] +
					MatPtr1[1] * MatPtr2[4 + j] +
					MatPtr1[2] * MatPtr2[8 + j] +
					MatPtr1[3] * MatPtr2[12 + j];
				DstPtr++;
			}
			MatPtr1+=4;
		}

		return NewMat;
	}

	/*
	* @brief Create and returns an inverse of the provided matrix.
	* 
	* @return A inverted copy of the matrix.
	*/
	Matrix4 Inverse() {
		float t0 = data[10] * data[15];
		float t1 = data[14] * data[11];
		float t2 = data[6] * data[15];
		float t3 = data[14] * data[7];
		float t4 = data[6] * data[11];
		float t5 = data[10] * data[7];
		float t6 = data[2] * data[15];
		float t7 = data[14] * data[3];
		float t8 = data[2] * data[11];
		float t9 = data[10] * data[3];
		float t10 = data[2] * data[7];
		float t11 = data[6] * data[3];
		float t12 = data[8] * data[13];
		float t13 = data[12] * data[9];
		float t14 = data[4] * data[13];
		float t15 = data[12] * data[5];
		float t16 = data[4] * data[9];
		float t17 = data[8] * data[5];
		float t18 = data[0] * data[13];
		float t19 = data[12] * data[1];
		float t20 = data[0] * data[9];
		float t21 = data[9] * data[1];
		float t22 = data[0] * data[5];
		float t23 = data[4] * data[1];

		Matrix4 Matrix;
		float* o = Matrix.data;

		o[0] = (t0 * data[5] + t3 * data[9] + t4 * data[13]) - (t1 * data[5] + t2 * data[9] + t5 * data[13]);
		o[1] = (t1 * data[1] + t6 * data[9] + t9 * data[13]) - (t0 * data[1] + t7 * data[9] + t8 * data[13]);
		o[2] = (t2 * data[1] + t7 * data[5] + t10 * data[13]) - (t3 * data[1] + t6 * data[5] + t11 * data[13]);
		o[3] = (t3 * data[1] + t8 * data[5] + t11 * data[9]) - (t4 * data[1] + t9 * data[5] + t10 * data[9]);

		float d = 1.0f / (data[0] * o[0] + data[4] * o[1] + data[8] * o[2] + data[12] * o[3]);

		o[0] = d * o[0];
		o[1] = d * o[1];
		o[2] = d * o[2];
		o[3] = d * o[3];
		o[4] = d * ((t1 * data[4] + t2 * data[8] + t5 * data[12]) - (t0 * data[4] + t3 * data[8] + t4 * data[12]));
		o[5] = d * ((t0 * data[0] + t7 * data[8] + t8 * data[12]) - (t1 * data[0] + t6 * data[8] + t9 * data[12]));
		o[6] = d * ((t3 * data[0] + t6 * data[4] + t11 * data[12]) - (t2 * data[0] + t7 * data[4] + t10 * data[12]));
		o[7] = d * ((t4 * data[0] + t9 * data[4] + t10 * data[8]) - (t5 * data[0] + t8 * data[4] + t11 * data[8]));

		o[8] = d * ((t12 * data[7] + t15 * data[11] + t16 * data[15]) - (t13 * data[7] + t14 * data[11] + t17 * data[15]));
		o[9] = d * ((t13 * data[3] + t18 * data[11] + t21 * data[15]) - (t12 * data[3] + t19 * data[11] + t20 * data[15]));
		o[10] = d * ((t14 * data[3] + t19 * data[7] + t22 * data[15]) - (t15 * data[3] + t18 * data[7] + t23 * data[15]));
		o[11] = d * ((t17 * data[3] + t20 * data[7] + t23 * data[11]) - (t16 * data[3] + t21 * data[7] + t22 * data[11]));

		o[12] = d * ((t14 * data[10] + t17 * data[14] + t13 * data[6]) - (t16 * data[14] + t12 * data[6] + t15 * data[10]));
		o[13] = d * ((t20 * data[14] + t12 * data[2] + t19 * data[10]) - (t18 * data[10] + t21 * data[14] + t13 * data[2]));
		o[14] = d * ((t18 * data[6] + t23 * data[14] + t15 * data[2]) - (t22 * data[14] + t14 * data[2] + t19 * data[6]));
		o[15] = d * ((t22 * data[10] + t16 * data[2] + t21 * data[6]) - (t20 * data[6] + t23 * data[10] + t17 * data[2]));

		return Matrix;
	}

	void SetTranslation(const Vec3& position) {
		data[12] = position.x;
		data[13] = position.y;
		data[14] = position.z;
	}

	Vec3 GetTranslation() const {
		return Vec3{ data[12], data[13], data[14] };
	}

	void SetScale(const Vec3& scale) {
		data[0] = scale.x;
		data[5] = scale.y;
		data[10] = scale.z;
	}

	Vec3 GetScale() const {
		return Vec3{ data[0], data[5], data[10] };
	}

	/*
	* @brief Returns a forward vector relative to the matrix.
	* 
	* @return A 3-Component directional vector.
	*/
	Vec3 Forward() const {
		Vec3 Vector;
		Vector.x = -data[2];
		Vector.y = -data[6];
		Vector.z = -data[10];
		return Vector.Normalize();
	}

	/*
	* @brief Returns a backward vector relative to the matrix.
	* 
	* @return A 3-component directional vector.
	*/
	Vec3 Backward() const {
		Vec3 Vector;
		Vector.x = data[2];
		Vector.y = data[6];
		Vector.z = data[10];
		return Vector.Normalize();
	}

	/*
	* @brief Returns a upward vector relative to the matrix
	* 
	* @return A 3-component directional vector
	*/
	Vec3 Up() const {
		Vec3 Vector;
		Vector.x = data[1];
		Vector.y = data[5];
		Vector.z = data[9];
		return Vector.Normalize();
	}

	/*
	* @brief Returns a down vector relative to the matrix
	*
	* @return A 3-component directional vector
	*/
	Vec3 Down() const {
		Vec3 Vector;
		Vector.x = -data[1];
		Vector.y = -data[5];
		Vector.z = -data[9];
		return Vector.Normalize();
	}

	/*
	* @brief Returns a left vector relative to the matrix
	*
	* @return A 3-component directional vector
	*/
	Vec3 Left() const {
		Vec3 Vector;
		Vector.x = -data[0];
		Vector.y = -data[4];
		Vector.z = -data[8];
		return Vector.Normalize();
	}

	/*
	* @brief Returns a right vector relative to the matrix
	*
	* @return A 3-component directional vector
	*/
	Vec3 Right() const {
		Vec3 Vector;
		Vector.x = data[0];
		Vector.y = data[4];
		Vector.z = data[8];
		return Vector.Normalize();
	}

public:
	/*
	* @brief Generate Identity Matrix
	* 
	* @return Identity matrix
	* { 1, 0, 0, 0,
	*   0, 1, 0, 0,
	*   0, 0, 1, 0,
	*   0, 0, 0, 1}
	*/
	static Matrix4 Identity() {
		Matrix4 Mat;
		Mat.data[0] = 1.0f;
		Mat.data[5] = 1.0f;
		Mat.data[10] = 1.0f;
		Mat.data[15] = 1.0f;
		return Mat;
	}

	/*
	* @brief Creates and return s an orthographic projection matrix. Typically used to render flat or 2D scene.
	* 
	* @param left The left side of the view frustum.
	* @param right The right side of the view frustum.
	* @param bottom The bottom side of the view frustum.
	* @param top The top side of the view frustum.
	* @param near_clip The near clipping plane distance.
	* @param far_clip The far clipping plane distance.
	* @return A new orthographic projection matrix.
	*/
	static Matrix4 Orthographic(float left, float right, float bottom, float top, float near_clip, float far_clip) {
		Matrix4 Matrix = Matrix4::Identity();

		float lr = 1.0f / (left - right);
		float bt = 1.0f / (bottom - top);
		float nf = 1.0f / (near_clip - far_clip);

		Matrix.data[0] = -2.0f * lr;
		Matrix.data[5] = -2.0f * bt;
		Matrix.data[10] = 2.0f * nf;

		Matrix.data[12] = (left + right) * lr;
		Matrix.data[13] = (top + bottom) * bt;
		Matrix.data[14] = (far_clip + near_clip) * nf;

		return Matrix;
	}

	/*
	* @brief Creates and returns a perspective matrix. Typically used to render 3D scenes.
	* 
	* @param fov_radians The field of view in radians.
	* @param aspect_ratio The aspect ratio.
	* @param near_clip The near clipping plane distance.
	* @param far_clip The far clipping plane distance.
	* @return A new perspective matrix.
	*/
	static Matrix4 Perspective(float fov_radians, float aspect_ratio, float near_clip, float far_clip) {
		float HalfFov = DTan(fov_radians * 0.5f);

		Matrix4 Matrix;
		Memory::Zero(Matrix.data, sizeof(float) * 16);

		Matrix.data[0] = 1.0f / (aspect_ratio * HalfFov);
		Matrix.data[5] = 1.0f / HalfFov;
		Matrix.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));
		Matrix.data[11] = -1.0f;
		Matrix.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));

		return Matrix;
	}

	/*
	* @brief Creates and returns a look-at matrix, or a matrix looking at target from the perspective of position.
	* 
	* @param position The position of the matrix.
	* @param target The look at target.
	* @param up The up vector.
	* @return A matrix looking at target from the perspective of position.
	*/
	static Matrix4 LookAt(Vec3 position, Vec3 target, Vec3 up) {
		Matrix4 Matrix;

		Vec3 AxisZ;
		AxisZ.x = target.x - position.x;
		AxisZ.y = target.y - position.y;
		AxisZ.z = target.z - position.z;
		AxisZ.Normalize();

		Vec3 AxisX = AxisZ.Cross(up).Normalize();
		Vec3 AxisY = AxisX.Cross(AxisZ);

		Matrix.data[0] = AxisX.x;
		Matrix.data[1] = AxisY.x;
		Matrix.data[2] = -AxisZ.x;
		Matrix.data[3] = 0;
		Matrix.data[4] = AxisX.y;
		Matrix.data[5] = AxisY.y;
		Matrix.data[6] = -AxisZ.y;
		Matrix.data[7] = 0;
		Matrix.data[8] = AxisX.z;
		Matrix.data[9] = AxisY.z;
		Matrix.data[10] = -AxisZ.z;
		Matrix.data[11] = 0;
		Matrix.data[12] = -AxisX.Dot(position);
		Matrix.data[13] = -AxisY.Dot(position);
		Matrix.data[14] = AxisZ.Dot(position);
		Matrix.data[15] = 1.0f;

		return Matrix;
	}

	static Matrix4 FromTranslation(Vec3 trans) {
		Matrix4 Ret = Matrix4::Identity();
		Ret.SetTranslation(trans);
		return Ret;
	}

	static Matrix4 FromScale(Vec3 scale) {
		Matrix4 Ret = Matrix4::Identity();
		Ret.SetScale(scale);
		return Ret;
	}

	/*
	* Euler
	*/
	static Matrix4 EulerX(float angle_radians) {
		Matrix4 Matrix;
		float c = DCos(angle_radians);
		float s = DSin(angle_radians);

		Matrix.data[5] = c;
		Matrix.data[6] = s;
		Matrix.data[9] = -s;
		Matrix.data[10] = c;

		return Matrix;
	}

	static Matrix4 EulerY(float angle_radians) {
		Matrix4 Matrix;
		float c = DCos(angle_radians);
		float s = DSin(angle_radians);

		Matrix.data[0] = c;
		Matrix.data[2] = -s;
		Matrix.data[8] = s;
		Matrix.data[10] = c;

		return Matrix;
	}

	static Matrix4 EulerZ(float angle_radians) {
		Matrix4 Matrix;
		float c = DCos(angle_radians);
		float s = DSin(angle_radians);

		Matrix.data[0] = c;
		Matrix.data[1] = s;
		Matrix.data[4] = -s;
		Matrix.data[5] = c;

		return Matrix;
	}

	static Matrix4 EulerXYZ(float x_radians, float y_radians, float z_radians) {
		Matrix4 Matrix;
		Matrix4 mx = Matrix4::EulerX(x_radians);
		Matrix4 my = Matrix4::EulerY(y_radians);
		Matrix4 mz = Matrix4::EulerZ(z_radians);

		Matrix = mx.Multiply(my);
		Matrix = Matrix.Multiply(mz);

		return Matrix;
	}

public:
	Matrix4& operator=(const Matrix4& mat) {
		data[0] = mat.data[0];
		data[1] = mat.data[1];
		data[2] = mat.data[2];
		data[3] = mat.data[3];
		data[4] = mat.data[4];
		data[5] = mat.data[5];
		data[6] = mat.data[6];
		data[7] = mat.data[7];
		data[8] = mat.data[8];
		data[9] = mat.data[9];
		data[10] = mat.data[10];
		data[11] = mat.data[11];
		data[12] = mat.data[12];
		data[13] = mat.data[13];
		data[14] = mat.data[14];
		data[15] = mat.data[15];

		return *this;
	}

	float& operator[](int i) {
		return data[i];
	}

	const float& operator[](int i) const {
		return data[i];
	}

	friend std::ostream& operator<<(std::ostream& os, const Matrix4& mat) {
		return os
			<< mat[0] << " " << mat[4] << " " << mat[8] << " " << mat[12] << "\n"
			<< mat[1] << " " << mat[5] << " " << mat[9] << " " << mat[13] << "\n"
			<< mat[2] << " " << mat[6] << " " << mat[10] << " " << mat[14] << "\n"
			<< mat[3] << " " << mat[7] << " " << mat[11] << " " << mat[15] << "\n";
	}
};

inline Matrix4 QuatToMatrix(Quaternion q) {
	Matrix4 Matrix = Matrix4::Identity();
	Quaternion n = q.Normalize();

	Matrix.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
	Matrix.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
	Matrix.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;

	Matrix.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
	Matrix.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
	Matrix.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;

	Matrix.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
	Matrix.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
	Matrix.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;

	return Matrix;
}

// Calculates a rotation matrix based on the quaternion and the passed in center point.
inline Matrix4 QuatToRotationMatrix(const Quaternion& q, const Vec3& center) {
	Matrix4 Matrix;
	float* o = Matrix.data;

	o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
	o[2] = 2.0f * ((q.x * q.z) - (q.y + q.w));
	o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

	o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
	o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
	o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
	o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

	o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
	o[9] = 2.0f * ((q.y * q.z) + (q.x * q.w));
	o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
	o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

	o[12] = 0.0f;
	o[13] = 0.0f;
	o[14] = 0.0f;
	o[15] = 1.0f;

	return Matrix;
}

inline Quaternion QuaternionFromAxisAngle(const Vec3& axis, float angle, bool normalize) {
	const float HalfAngle = 0.5f * angle;
	float s = DSin(HalfAngle);
	float c = DCos(HalfAngle);

	Quaternion q{ s * axis.x, s * axis.y, s * axis.z, c };
	if (normalize) {
		q.Normalize();
	}

	return q;
}

inline Quaternion QuaternionSlerp(Quaternion q0, Quaternion q1, float percentage) {
	Quaternion Quat;

	Quaternion v0 = q0.Normalize();
	Quaternion v1 = q1.Normalize();

	// Compute the cosine of the angle between the two vectors;
	float dot = v0.Dot(v1);

	// If the dot product is negative, slerp won't take
	// the shorter path. Note that v1 and -v1 are equivalent when the negation is applied to all four components.
	// Fix by reversing one quaternion
	if (dot < 0.0f) {
		v1.x = -v1.x;
		v1.y = -v1.y;
		v1.z = -v1.z;
		v1.w = -v1.w;
		dot = -dot;
	}

	const float DOT_THRESHOLD = 0.9995f;
	if (dot > DOT_THRESHOLD) {
		// If the inputs are too close for comfort, linearly interpolate and normalize the result.
		Quat = Quaternion{
			v0.x + ((v1.x - v0.x) * percentage),
			v0.y + ((v1.y - v0.y) * percentage),
			v0.z + ((v1.z - v0.z) * percentage),
			v0.w + ((v1.w - v0.w) * percentage)
		};

		return Quat.Normalize();
	}

	// Since dot is in range[0, DOT_THRESHOLD], acos is safe.
	float theta_0 = DCos(dot);
	float theta = theta_0 * percentage;
	float sin_theta = DSin(theta);
	float sin_theta_0 = DSin(theta_0);

	float s0 = DCos(theta) - dot * sin_theta / sin_theta_0;
	float s1 = sin_theta / sin_theta_0;

	return Quaternion{
		(v0.x * s0) + (v1.x * s1),
		(v0.y * s0) + (v1.y * s1),
		(v0.z * s0) + (v1.z * s1),
		(v0.w * s0) + (v1.w * s1)
	};

}

struct Vertex {
	Vec3 position;
	Vec3 normal;
	Vec2 texcoord;
	Vec4 color;
	Vec4 tangent;
};

struct Vertex2D {
	Vec2 position;
	Vec2 texcoord;
};
