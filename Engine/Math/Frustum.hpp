#pragma once
#include "Vector.hpp"

/**
 * @brief Represents the extents of a 2D object.
 */
template<typename T>
struct TExtents2D {
	TVector2<T> min;
	TVector2<T> max;
};

/**
 * @brief Represents the extents of a 3D object.
 */
template<typename T>
struct TExtents3D {
	TVector3<T> min;
	TVector3<T> max;
};

// Frustum culling
enum class FrustumCullMode {
	eSphere_Cull,
	eAABB_Cull
};

class DAPI Plane3D {
public:
	Plane3D() : Normal(0), Distance(0.0f) {}
	Plane3D(TVector3<float> p1, TVector3<float> Norm) {
		Normal = Norm.Normalized();
		Distance = Normal.Dot(p1);
	}

	/**
	 * @brief Obtains the signed distance between the plane p and the provided position.
	 *
	 * @param position A constant pointer to a position.
	 * @return The signed distance from the point to the plane.
	 */
	float SignedDistance(const TVector3<float>& position) const {
		return Normal.Dot(position) - Distance;
	}

	/**
	 * @brief Indicates if plane p intersects a sphere constructed via center and radius.
	 *
	 * @param center A constant pointer to a position representing the center of a sphere.
	 * @param radius The radius of the sphere.
	 * @return True if the sphere intersects the plane; otherwise false.
	 */
	bool IntersectsSphere(const TVector3<float>& center, float radius) {
		return SignedDistance(center) > -radius;
	}

	/**
	 * @brief Indicates if plane p intersects an axis-aligned bounding box constructed via center and extents.
	 *
	 * @param center A constant pointer to a position representing the center of an axis-aligned bounding box.
	 * @param extents The half-extents of an axis-aligned bounding box.
	 * @return True if the axis-aligned bounding box intersects the plane; otherwise false.
	 */
	bool IntersectsAABB(const TVector3<float>& center, const TVector3<float>& extents) {
		float r = extents.x * Dabs(Normal.x) +
			extents.y * Dabs(Normal.y) +
			extents.z * Dabs(Normal.z);
		return -r <= SignedDistance(center);
	}

public:
	TVector3<float> Normal;
	float Distance;
};

struct Frustum {
public:
	Frustum() {}

	/**
	 * @brief Creates and returns a frustum based on the provided position, direction vectors, aspect, field of view,
	 * and near/far clipping planes (typically obtained from a camera). This is typically used for frustum culling.
	 *
	 * @param position A constant pointer to the position to be used.
	 * @param forward A constant pointer to the forward vector to be used.
	 * @param right A constant pointer to the right vector to be used.
	 * @param up A constant pointer to the up vector to be used.
	 * @param aspect The aspect ratio.
	 * @param fov The vertical field of view.
	 * @param near The near clipping plane distance.
	 * @param far The far clipping plane distance.
	 */
	Frustum(const TVector3<float>& position, const TVector3<float>& forward, const TVector3<float>& right, const TVector3<float>& up, float aspect, float fov, float near, float far) {
		const float HalfV = far * tanf(fov * 0.5f);
		const float HalfH = HalfV * aspect;
		const TVector3<float> fwd = forward;
		const TVector3<float> ForwardFar = fwd * far;
		const TVector3<float> RightHalfH = right * HalfH;
		const TVector3<float> UpHalfV = up * HalfV;

		// near, far, right, left, bottom. top
		Sides[0] = Plane3D(position + fwd * near, fwd);
		Sides[1] = Plane3D(position + ForwardFar, fwd * -1.0f);
		Sides[2] = Plane3D(position, up.Cross(ForwardFar + RightHalfH));
		Sides[3] = Plane3D(position, (ForwardFar - RightHalfH).Cross(up));
		Sides[4] = Plane3D(position, right.Cross(ForwardFar - UpHalfV));
		Sides[5] = Plane3D(position, (ForwardFar + UpHalfV).Cross(right));
	}

	/**
	 * @brief Indicates if the frustum intersects (or contains) a sphere constructed via center and radius.
	 *
	 * @param center A constant pointer to a position representing the center of a sphere.
	 * @param radius The radius of the sphere.
	 * @return True if the sphere is intersected by or contained within the frustum f; otherwise false.
	 */
	bool IntersectsSphere(const TVector3<float>& center, float radius) {
		for (unsigned char i = 0; i < 6; ++i) {
			if (!Sides[i].IntersectsSphere(center, radius)) {
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Indicates if frustum f intersects an axis-aligned bounding box constructed via center and extents.
	 *
	 * @param center A constant pointer to a position representing the center of an axis-aligned bounding box.
	 * @param extents The half-extents of an axis-aligned bounding box.
	 * @return True if the axis-aligned bounding box is intersected by or contained within the frustum f; otherwise false.
	 */
	bool IntersectsAABB(const TVector3<float>& center, const TVector3<float>& extents) {
		for (unsigned char i = 0; i < 6; ++i) {
			if (!Sides[i].IntersectsAABB(center, extents)) {
				return false;
			}
		}

		return true;
	}

public:
	// Top bottom right left far near
	Plane3D Sides[6];
};
