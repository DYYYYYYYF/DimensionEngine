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

	// eular(Yaw, Pitch, Roll) = (X, Y, Z)
	TQuaternion(const TVector3<T>& euler) {
		T Roll = Deg2Rad(euler.z);
		T Pitch = Deg2Rad(euler.y);
		T Yaw = Deg2Rad(euler.x);

		T CosRoll = DCos(Roll / T(2));
		T SinRoll = DSin(Roll / T(2));
		T CosPitch = DCos(Pitch / T(2));
		T SinPitch = DSin(Pitch / T(2));
		T CosYaw = DCos(Yaw / T(2));
		T SinYaw = DSin(Yaw / T(2));

		// ZYX轴序计算（保持不变）
		w = CosRoll * CosPitch * CosYaw + SinRoll * SinPitch * SinYaw;
		x = CosRoll * CosPitch * SinYaw - SinRoll * SinPitch * CosYaw;
		y = CosRoll * SinPitch * CosYaw + SinRoll * CosPitch * SinYaw;
		z = SinRoll * CosPitch * CosYaw - CosRoll * SinPitch * SinYaw;
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
		TQuaternion<T> normalized = Normalize();

		T xx = normalized.x * normalized.x;
		T yy = normalized.y * normalized.y;
		T zz = normalized.z * normalized.z;
		T xy = normalized.x * normalized.y;
		T xz = normalized.x * normalized.z;
		T yz = normalized.y * normalized.z;
		T wx = normalized.w * normalized.x;
		T wy = normalized.w * normalized.y;
		T wz = normalized.w * normalized.z;

		Matrix.data[0] = static_cast<T>(1) - static_cast<T>(2) * (yy + zz);
		Matrix.data[1] = static_cast<T>(2) * (xy + wz);
		Matrix.data[2] = static_cast<T>(2) * (xz - wy);

		Matrix.data[4] = static_cast<T>(2) * (xy - wz);
		Matrix.data[5] = static_cast<T>(1) - static_cast<T>(2) * (xx + zz);
		Matrix.data[6] = static_cast<T>(2) * (yz + wx);

		Matrix.data[8] = static_cast<T>(2) * (xz + wy);
		Matrix.data[9] = static_cast<T>(2) * (yz - wx);
		Matrix.data[10] = static_cast<T>(1) - static_cast<T>(2) * (xx + yy);

		return Matrix;
	}

	// Calculates a rotation matrix based on the quaternion and the passed in center point.
	inline TMatrix4<T> ToRotationMatrix(const TVector3<T>& center) {
		TMatrix4<T> Matrix = ToRotationMatrix();

		// 列主序矩阵布局：
		// [0  4  8  12]   [r00 r01 r02  tx]
		// [1  5  9  13] = [r10 r11 r12  ty]  
		// [2  6  10 14]   [r20 r21 r22  tz]
		// [3  7  11 15]   [ 0   0   0   1]

		// 计算 R * center（矩阵乘向量）
		T rotated_x = Matrix.data[0] * center.x + Matrix.data[4] * center.y + Matrix.data[8] * center.z;
		T rotated_y = Matrix.data[1] * center.x + Matrix.data[5] * center.y + Matrix.data[9] * center.z;
		T rotated_z = Matrix.data[2] * center.x + Matrix.data[6] * center.y + Matrix.data[10] * center.z;

		// 计算平移：T = center - R * center
		T tx = center.x - rotated_x;
		T ty = center.y - rotated_y;
		T tz = center.z - rotated_z;

		// 设置平移部分（列主序：第4列）
		Matrix.data[12] = tx;
		Matrix.data[13] = ty;
		Matrix.data[14] = tz;
		Matrix.data[15] = static_cast<T>(1);

		return Matrix;
	}

	inline static TQuaternion Slerp(TQuaternion<T> q0, TQuaternion<T> q1, T t) {
		// 参数验证
		if (t <= static_cast<T>(0)) {
			return q0;
		}
		if (t >= static_cast<T>(1)) {
			return q1;
		}

		// 获取标准化的四元数（使用 Normalized() 而不是 Normalize()）
		TQuaternion v0 = q0.Normalize();
		TQuaternion v1 = q1.Normalize();

		// 计算点积
		T dot = v0.Dot(v1);

		// 如果点积为负，选择更短的路径
		if (dot < static_cast<T>(0)) {
			v1 = TQuaternion(-v1.x, -v1.y, -v1.z, -v1.w);
			dot = -dot;
		}

		const T DOT_THRESHOLD = static_cast<T>(0.9995);

		// 如果四元数非常接近，使用线性插值避免数值不稳定
		if (dot > DOT_THRESHOLD) {
			TQuaternion result(
				v0.x + (v1.x - v0.x) * t,  // 现在使用正确的变量 t
				v0.y + (v1.y - v0.y) * t,
				v0.z + (v1.z - v0.z) * t,
				v0.w + (v1.w - v0.w) * t
			);
			return result.Normalize();
		}

		// 球面线性插值
		T theta_0 = std::acos(std::clamp(dot, static_cast<T>(-1), static_cast<T>(1)));  // 防止 acos 域错误
		T theta = theta_0 * t;  // 现在使用正确的变量 t
		T sin_theta = std::sin(theta);
		T sin_theta_0 = std::sin(theta_0);

		// 防止除零
		if (std::abs(sin_theta_0) < std::numeric_limits<T>::epsilon()) {
			return v0;  // 如果 sin_theta_0 接近 0，返回起始四元数
		}

		T s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
		T s1 = sin_theta / sin_theta_0;

		return TQuaternion(
			v0.x * s0 + v1.x * s1,
			v0.y * s0 + v1.y * s1,
			v0.z * s0 + v1.z * s1,
			v0.w * s0 + v1.w * s1
		);
	}

	inline TVector3<T> ToEuler() const {
		T Roll = T(0);  
		T Pitch = T(0); 
		T Yaw = T(0);   

		// ZYX轴序（Tait-Bryan angles）的四元数到欧拉角转换

		// Roll 
		T SinR_CosP = T(2) * (w * z + x * y);
		T CosR_CosP = T(1) - T(2) * (y * y + z * z);
		Roll = DArcTan2(SinR_CosP, CosR_CosP);

		// Pitch 
		T SinP = T(2) * (w * y - z * x);
		if (Dabs(SinP) >= T(1)) {
			// 万向锁情况处理
			Pitch = copysign(T(D_PI / 2), SinP);
		}
		else {
			Pitch = asin(SinP);
		}

		// Yaw 
		T SinY_CosP = T(2) * (w * x + y * z);
		T CosY_CosP = T(1) - T(2) * (x * x + y * y);
		Yaw = DArcTan2(SinY_CosP, CosY_CosP);

		// (Yaw, Pitch, Roll)
		return TVector3<T>(Yaw, Pitch, Roll);
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
	TQuaternion& Normalize() {
		T len = Length();
		if (len < std::numeric_limits<T>::epsilon()) {
			x = y = z = static_cast<T>(0);
			w = static_cast<T>(1);
		}
		else {
			T invLen = static_cast<T>(1) / len;
			x *= invLen;
			y *= invLen;
			z *= invLen;
			w *= invLen;
		}
		return *this;
	}

	TQuaternion<T> Normalize() const {
		T len = Length();
		if (len < std::numeric_limits<T>::epsilon()) {
			return TQuaternion(static_cast<T>(0), static_cast<T>(0),
				static_cast<T>(0), static_cast<T>(1));
		}
		T invLen = static_cast<T>(1) / len;
		return TQuaternion(x * invLen, y * invLen, z * invLen, w * invLen);
	}

	float Dot(const TQuaternion<T>& v) {
		return x * v.x + y * v.y + z * v.z + w * v.w;
	}

	friend std::ostream& operator<<(std::ostream& os, const TQuaternion<T>& q) {
		return os << q.x << " " << q.y << " " << q.z << " " << q.w;
	}
};
