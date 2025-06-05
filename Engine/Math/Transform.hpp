#pragma once
#include "MathTypes.hpp"

class DAPI alignas(16) Transform {
public:
	/**
	 * @brief Creates and returns a new transform, using a zero vector for position,
	 * identity quaternion for rotation, and a one vector for scale. Also has a nullptr
	 * parent. Marked dirty by default.
	 */
	Transform();

	Transform(const Transform& trans);

	template<typename T>
	Transform(const std::vector<T>& dat) {
		SetLocation(Vector3((float)dat[12], (float)dat[13], (float)dat[14]));

		float ScaleX = Vector3((float)dat[0], (float)dat[1], (float)dat[2]).Length();
		float ScaleY = Vector3((float)dat[4], (float)dat[5], (float)dat[6]).Length();
		float ScaleZ = Vector3((float)dat[8], (float)dat[9], (float)dat[10]).Length();
		SetScale(Vector3(ScaleX, ScaleY, ScaleZ));

		Matrix4 Rotation = Matrix4::Identity();
		Rotation[0] = (float)dat[0] / ScaleX;
		Rotation[1] = (float)dat[1] / ScaleX;
		Rotation[2] = (float)dat[2] / ScaleX;
		Rotation[4] = (float)dat[4] / ScaleY;
		Rotation[5] = (float)dat[5] / ScaleY;
		Rotation[6] = (float)dat[6] / ScaleY;
		Rotation[8] = (float)dat[8] / ScaleZ;
		Rotation[9] = (float)dat[9] / ScaleZ;
		Rotation[10] = (float)dat[10] / ScaleZ;

		Quaternion Quat = MatrixToQuat(Rotation);
		SetQuaternion(Quat);
	}

	/**
	 * @brief Creates a transform from the given position.
	 * Uses a zero rotation and a one scale.
	 * 
	 * @param position The position to be used.
	 */
	Transform(const Vector3& position);

	/**
	 * @brief Creates a transform from the given rotation.
	 * Uses one scale.
	 *
	 * @param rotation The rotation to be used.
	 */
	Transform(const Quaternion& rotation);

	/**
	 * @brief Creates a transform from the given rotation and position.
	 * Uses one scale.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 */
	Transform(const Vector3& position, const Quaternion& rotation);

	/**
	 * @brief Creates a transform.
	 *
	 * @param position The position to be used.
	 * @param rotation The rotation to be used.
	 * @param scale The scale to be used.
	 */
	Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);

public:
	void SetLocation(Vector3 pos) { vPosition = pos; bIsDirty = true; }
	const Vector3& GetLocation() const { return vPosition; }

	void SetScale(Vector3 scale) { vScale = scale; bIsDirty = true; }
	const Vector3& GetScale() const { return vScale; }

	void SetQuaternion(Quaternion quat) { vRotation = quat; bIsDirty = true; }
	const Quaternion& GetQuaternion() const { return vRotation; }


	void Translate(const Vector3& translation);
	void Rotate(const Quaternion& rotation);
	void Scale(const Vector3& scale);

	void SetPR(const Vector3& pos, const Quaternion& rotation);
	void SetPRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale);
	void TransformRotate(const Vector3& translation, const Quaternion& rotation);

	Matrix4 GetLocal() const;
	Matrix4 GetWorldMatrix() const;
	Matrix4 GetInverseWorldMatrix() const;
	Vector3 TransformPoint(const Vector3& point) const;
	Vector3 TransformDirection(const Vector3& direction) const;
	Vector3 InverseTransformPoint(const Vector3& point) const;

	bool IsDirty() const { return bIsDirty; }
	void UpdateLocal() const;

private:
	Vector3 vPosition;
	Quaternion vRotation;
	Vector3 vScale;

	mutable bool bIsDirty;
	mutable Matrix4 Local;
	mutable Matrix4 InverseLocal;
	mutable bool bInverseDirty;

};