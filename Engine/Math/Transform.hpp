#pragma once
#include "MathTypes.hpp"

class DAPI Transform {
public:
	/**
	 * @brief Creates and returns a new transform, using a zero vector for position,
	 * identity quaternion for rotation, and a one vector for scale. Also has a nullptr
	 * parent. Marked dirty by default.
	 */
	Transform();

	Transform(const Transform& trans);

	/**
	 * @brief Creates a transform from the given position.
	 * Uses a zero rotation and a one scale.
	 * 
	 * @param position The position to be used.
	 */
	Transform(Vec3 position);

	/**
	 * @brief Creates a transform from the given rotation.
	 * Uses one scale.
	 *
	 * @param rotation The rotation to be used.
	 */
	Transform(Quaternion rotation);

	/**
	 * @brief Creates a transform from the given rotation and position.
	 * Uses one scale.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 */
	Transform(Vec3 position, Quaternion rotation);

	/**
	 * @brief Creates a transform.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 * @param scale The scale to be used.
	 */
	Transform(Vec3 position, Quaternion rotation, Vec3 scale);

public:
	void SetParentTransform(Transform* t) { Parent = t; IsDirty = true; }
	Transform* GetParentTransform() { return Parent; }

	void SetPosition(Vec3 pos) { vPosition = pos; IsDirty = true; }
	const Vec3& GetPosition() const { return vPosition; }

	void SetScale(Vec3 scale) { vScale = scale; IsDirty = true; }
	const Vec3& GetScale() const { return vScale; }

	void SetRotation(Quaternion quat) { vRotation = quat; IsDirty = true; }
	const Quaternion& GetRotation() const { return vRotation; }


	void Translate(const Vec3& translation);
	void Rotate(const Quaternion& rotation);
	void Scale(const Vec3& scale);

	void SetPR(const Vec3& pos, const Quaternion& rotation);
	void SetPRS(const Vec3& pos, const Quaternion& rotation, const Vec3& scale);
	void TransformRotate(const Vec3& translation, const Quaternion& rotation);

	Matrix4 GetLocal();
	Matrix4 GetWorldTransform();

private:
	Vec3 vPosition;
	Quaternion vRotation;
	Vec3 vScale;
	bool IsDirty;
	Matrix4 Local;
	class Transform* Parent;
};