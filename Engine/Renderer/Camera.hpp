#pragma once

#include "Math/MathTypes.hpp"

/**
 * @brief Represents a camera that can be used for a variety of things, expecially rendering.
 * Ideally, these are created and managed by the camera system.
 */
class DAPI Camera {
public:
	/**
	 * @brief Creates a new camera with default zero position and rotation, and view
	 * identity matrix. Ideally, the camera system should be used to create this instead
	 * of doing so directly.
	 */
	Camera();
	Camera(uint32_t id);

public:
	/**
	 * @brief Defaults the camera to default zero rotation and position, and view matrix to identity.
	 */
	void Reset();

	/**
	 * @brief Set camera's position.
	 * 
	 * @param pos The position of the camera.
	 */
	void SetPosition(Vector3 pos);

	/**
	 * @brief Get a copy of camera's position.
	 *
	 * @return The position of the camera.
	 */
	Vector3 GetPosition();

	/**
	 * @brief Set camera's rotation in Eular angles.
	 *
	 * @param pos The rotation of the camera.
	 */
	void SetEulerAngles(Vector3 eular);

	/**
	 * @brief Get a copy of camera's rotation.
	 *
	 * @return The rotation of the camera.
	 */
	Vector3 GetEulerAngles();

	/**
	 * @brief Set camera's view matrix.
	 *
	 * @param mat The view matrix of the camera.
	 */
	void SetViewMatrix(const Matrix4& mat);

	/**
	 * @brief Obtains a copy of the camera's view matrix. If camera is dirty,
	 * a new one is created, set and returned.
	 * 
	 * @return A copy of the up-to-date view matrix.
	 */
	Matrix4 GetViewMatrix();

	/**
	 * @brief Move camera forward.
	 */
	void MoveForward(float amount);

	/**
	 * @brief Move camera backward.
	 */
	void MoveBackward(float amount);

	/**
	 * @brief Move camera left.
	 */
	void MoveLeft(float amount);

	/**
	 * @brief Move camera right.
	 */
	void MoveRight(float amount);

	/**
	 * @brief Move camera up.
	 */
	void MoveUp(float amount);

	/**
	 * @brief Move camera down.
	 */
	void MoveDown(float amount);

	/**
	 * @brief Rotate camera around axis y.
	 */
	void RotateYaw(float amount);

	/**
	 * @brief Rotate camera around axis x.
	 */
	void RotatePitch(float amount);

	/**
	 * @brief Get forward vector of view.
	 */
	Vector3 Forward() { 
		const Matrix4& View = GetViewMatrix();
		return View.Forward();
	}

	/**
	 * @brief Get backward vector of view.
	 */
	Vector3 Backward() { 
		const Matrix4& View = GetViewMatrix();
		return View.Backward();
	}

	/**
	 * @brief Get left vector of view.
	 */
	Vector3 Left() {
		const Matrix4& View = GetViewMatrix();
		return View.Left();
	}

	/**
	 * @brief Get right vector of view.
	 */
	Vector3 Right() {
		const Matrix4& View = GetViewMatrix();
		return View.Right();
	}

	/**
	 * @brief Returns a copy of the camera's up vector.
	 *
	 * @return A copy of the camera's up vector.
	 */
	Vector3 Up() {
		const Matrix4& View = GetViewMatrix();
		return View.Up();
	}

public:
	uint32_t GetID() const { return ID; }
	void SetID(uint32_t id) { ID = id; }

	uint32_t GetReferenceCount() const { return ReferenceCount; }
	void SetReferenceCount(uint32_t count) { ReferenceCount = count; }
	void IncreaseReferenceCount(uint32_t count = 1) { ReferenceCount += count; }
	void DecreaseReferenceCount(uint32_t count = 1) { ReferenceCount -= count; }

private:
	/**
	 * @brief The symbol of this camera.
	 */
	uint32_t ID;

	/**
	 * @brief The reference count of this camera.
	 */
	uint32_t ReferenceCount;

	/**
	 * @brief The position of this camera.
	 * NOTE: Do not set this directly. Use SetPosition() instead so the view
	 * matrix will be recalculated when needed.
	 */
	Vector3 Position;

	/**
	 * @brief The rotation of this camera using Euler angles (pitch, yaw, roll).
	 * NOTO: Do not set this directly. Use SetEulerAngles() instead so the view
	 * matrix will be recalculated when needed.
	 */
	Vector3 EulerRotation;
	Quaternion CameraQuaternion;

	/** @brief Internal flag used to determine when the view matrix needs to be rebuilt. */
	bool IsDirty;

	/**
	 * @brief The view matrix of this camer.
	 * NOTO: Do not get this directly. Use GetViewMatrix() instead so the view
	 * matrix will be recalculated when needed.
	 */
	Matrix4 ViewMatrix;
};