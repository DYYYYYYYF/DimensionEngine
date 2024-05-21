#pragma once
#include "MathTypes.hpp"

class Transform {
public:
	/**
	 * @brief Creates and returns a new transform, using a zero vector for position,
	 * identity quaternion for rotation, and a one vector for scale. Also has a nullptr
	 * parent. Marked dirty by default.
	 */
	DAPI Transform();

	DAPI Transform(const Transform& trans);

	/**
	 * @brief Creates a transform from the given position.
	 * Uses a zero rotation and a one scale.
	 * 
	 * @param position The position to be used.
	 */
	DAPI Transform(Vec3 position);

	/**
	 * @brief Creates a transform from the given rotation.
	 * Uses one scale.
	 *
	 * @param rotation The rotation to be used.
	 */
	DAPI Transform(Quaternion rotation);

	/**
	 * @brief Creates a transform from the given rotation and position.
	 * Uses one scale.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 */
	DAPI Transform(Vec3 position, Quaternion rotation);

	/**
	 * @brief Creates a transform.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 * @param scale The scale to be used.
	 */
	DAPI Transform(Vec3 position, Quaternion rotation, Vec3 scale);

public:
	DAPI void SetParentTransform(Transform* t) { Parent = t; IsDirty = true; }
	DAPI Transform* GetParentTransform() { return Parent; }

	DAPI void SetPosition(Vec3 pos) { vPosition = pos; IsDirty = true; }
	DAPI const Vec3& GetPosition() const { return vPosition; }

	DAPI void SetScale(Vec3 scale) { vScale = scale; IsDirty = true; }
	DAPI const Vec3& GetScale() const { return vScale; }

	DAPI void SetRotation(Quaternion quat) { vRotation = quat; IsDirty = true; }
	DAPI const Quaternion& GetRotation() const { return vRotation; }


	DAPI void Translate(Vec3 translation);
	DAPI void Rotate(Quaternion rotation);
	DAPI void Scale(Vec3 scale);

	DAPI void SetPR(Vec3 pos, Quaternion rotation);
	DAPI void SetPRS(Vec3 pos, Quaternion rotation, Vec3 scale);
	DAPI void TransformRotate(Vec3 translation, Quaternion rotation);

	DAPI Matrix4 GetLocal();
	DAPI Matrix4 GetWorldTransform();

private:
	Vec3 vPosition;
	Quaternion vRotation;
	Vec3 vScale;
	bool IsDirty;
	Matrix4 Local;
	class Transform* Parent;
};