#pragma once
#include "Vector.hpp"
#include "Matrix.hpp"

//-----------------------------------------------
// Quaternion
//-----------------------------------------------
template<typename T>
struct TQuaternion {
	union
	{
        // Used for SIMD operations
#if defined(SIMD_SUPPORTED_NEON)
        alignas(16) float32x4_t data;
#elif defined(SIMD_SUPPORTED)
		alignas(16) __m128 data;
#endif
		// An array of x, y, z, w
		alignas(16) T elements[4] = { 0.0f };
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
	TQuaternion() {
		x = 0.0;
		y = 0.0;
		z = 0.0;
		w = 1.0;
	}

	TQuaternion(T x, T y, T z, T w) {
		r = x;
		g = y;
		b = z;
		a = w;
	}

	TQuaternion(const TVector3<T>& euler) {
		T EulerX = Deg2Rad(euler.x);
		T EulerY = Deg2Rad(euler.y);
		T EulerZ = Deg2Rad(euler.z);

		T CosPitch = DCos(EulerX / 2.0f);
		T SinPitch = DSin(EulerX / 2.0f);
		T CosYaw = DCos(EulerY / 2.0f);
		T SinYaw = DSin(EulerY / 2.0f);
		T CosRoll = DCos(EulerZ / 2.0f);
		T SinRoll = DSin(EulerZ / 2.0f);

		// Calculate quaternion
		w = CosPitch * CosYaw * CosRoll + SinPitch * SinYaw * SinRoll;
		x = SinPitch * CosYaw * CosRoll - CosPitch * SinYaw * SinRoll;
		y = CosPitch * SinYaw * CosRoll + SinPitch * CosYaw * SinRoll;
		z = CosPitch * CosYaw * SinRoll - SinPitch * SinYaw * CosRoll;
	}

	TQuaternion(const TVector3<T>& axis, T angle, bool normalize = true) {
		T HalfAngle = 0.5f * angle;
		T s = DSin(HalfAngle);
		T c = DCos(HalfAngle);

		x = s * axis.x;
		y = s * axis.y;
		z = s * axis.z;
		w = c;

		if (normalize) {
			Normalize();
		}
	}

	TQuaternion(const TQuaternion& quat) {
		x = quat.x;
		y = quat.y;
		z = quat.z;
		w = quat.w;
	}

public:
	inline void UpdateByEuler(const TVector3<T>& euler) {
		T EulerX = Deg2Rad(euler.x);
		T EulerY = Deg2Rad(euler.y);
		T EulerZ = Deg2Rad(euler.z);

		T CosPitch = DCos(EulerX / 2.0f);
		T SinPitch = DSin(EulerX / 2.0f);
		T CosYaw = DCos(EulerY / 2.0f);
		T SinYaw = DSin(EulerY / 2.0f);
		T CosRoll = DCos(EulerZ / 2.0f);
		T SinRoll = DSin(EulerZ / 2.0f);

		// Calculate quaternion
		w = CosPitch * CosYaw * CosRoll + SinPitch * SinYaw * SinRoll;
		x = SinPitch * CosYaw * CosRoll - CosPitch * SinYaw * SinRoll;
		y = CosPitch * SinYaw * CosRoll + SinPitch * CosYaw * SinRoll;
		z = CosPitch * CosYaw * SinRoll - SinPitch * SinYaw * CosRoll;
	}

	inline TMatrix4<T> ToRotationMatrix() const {
		TMatrix4<T> Matrix = TMatrix4<T>::Identity();
		Normalize();

		Matrix.data[0] = 1.0f - 2.0f * (y * y + z * z);
		Matrix.data[1] = 2.0f * (x * y + z * w);
		Matrix.data[2] = 2.0f * (x * z - y * w);

		Matrix.data[4] = 2.0f * (x * y - z * w);
		Matrix.data[5] = 1.0f - 2.0f * (x * x + z * z);
		Matrix.data[6] = 2.0f * (y * z + x * w);

		Matrix.data[8] = 2.0f * (x * z + y * w);
		Matrix.data[9] = 2.0f * (y * z - x * w);
		Matrix.data[10] = 1.0f - 2.0f * (x * x + y * y);
		
		return Matrix;
	}

	// Calculates a rotation matrix based on the quaternion and the passed in center point.
	inline TMatrix4<T> ToRotationMatrix(const TVector3<T>& center) {
		TMatrix4<T> Matrix = ToRotationMatrix();

		// 齐次坐标的平移部分：旋转中心变换后得到的平移
		Matrix.data[3] = -center.x * Matrix.data[0] - center.y * Matrix.data[1] - center.z * Matrix.data[2];
		Matrix.data[7] = -center.x * Matrix.data[4] - center.y * Matrix.data[5] - center.z * Matrix.data[6];
		Matrix.data[11] = -center.x * Matrix.data[8] - center.y * Matrix.data[9] - center.z * Matrix.data[10];

		// 最后一行保持齐次坐标：0, 0, 0, 1
		Matrix.data[12] = 0.0f;
		Matrix.data[13] = 0.0f;
		Matrix.data[14] = 0.0f;
		Matrix.data[15] = 1.0f;

		return Matrix;
	}

	inline TQuaternion QuaternionSlerp(TQuaternion<T> q0, TQuaternion<T> q1, float percentage) {
		TQuaternion<T> Quat;
		TQuaternion<T> v0 = q0.Normalize();
		TQuaternion<T> v1 = q1.Normalize();

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
			Quat = TQuaternion<T>{
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

		return TQuaternion<T>{
			(v0.x* s0) + (v1.x * s1),
				(v0.y* s0) + (v1.y * s1),
				(v0.z* s0) + (v1.z * s1),
				(v0.w* s0) + (v1.w * s1)
		};
	}

	inline TVector3<T> ToEuler() const {
		T Pitch = 0.0f;
		T Yaw = 0.0f;
		T Roll = 0.0f;

		// Pitch
		T SinR_CosP = 2.0f * (w * x + y * z);
		T CosR_CosP = 1.0f - 2.0f * (x * x + y * y);
		Pitch = DArcTan2(SinR_CosP, CosR_CosP);

		// Yaw
		T Sinp = 2.0f * (w * y - x * z);
		if (Dabs(Sinp) >= 1.0f) {
			Yaw = copysign(D_PI / 2.0f, Sinp);
		}
		else {
			Yaw = asin(Sinp);
		}

		// Roll
		T SinY_CosP = 2.0f * (w * z + x * y);
		T CosY_CosP = 1.0f - 2.0f * (y * y + z * z);
		Roll = DArcTan2(SinY_CosP, CosY_CosP);

		return TVector3<T>(Pitch, Yaw, Roll);
	}

	inline float Normal() const {
		return Dsqrt(
			x * x +
			y * y +
			z * z +
			w * w
		);
	}

	// 共轭
	inline void Conjugate() {
		x *= -1;
		y *= -1;
		z *= -1;
	}

	inline void Inverse() {
		Conjugate();
	}

	inline TQuaternion<T> Multiply(const TQuaternion<T>& q) const {
		Quaternion NewQuat;
		NewQuat.x = x * q.w +
			y * q.z -
			z * q.y +
			w * q.x;
		NewQuat.y = -x * q.z +
			y * q.w +
			z * q.x +
			w * q.y;
		NewQuat.z = x * q.y -
			y * q.x +
			z * q.w +
			w * q.z;
		NewQuat.w = -x * q.x -
			y * q.y -
			z * q.z +
			w * q.w;

		return NewQuat;
	}

	float LengthSquared() const { return x * x + y * y + z * z + w * w; }
	float Length() const { return Dsqrt(LengthSquared()); }
	TQuaternion<T> Normalize() {
		x /= Length();
		y /= Length();
		z /= Length();
		w /= Length();

		return *this;
	}

	TQuaternion<T> Normalize() const {
		T l = Length();
		if (l < FLT_MIN) {
			return Quaternion();
		}

		return TQuaternion(x / l, y / l, z / l, w / l);
	}

	float Dot(const TQuaternion<T>& v) {
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	friend std::ostream& operator<<(std::ostream& os, const TQuaternion<T>& q) {
		return os << q.x << " " << q.y << " " << q.z << " " << q.w;
	}
};
