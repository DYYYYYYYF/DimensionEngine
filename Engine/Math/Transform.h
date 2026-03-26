#pragma once

#include "Math/MathTypes.hpp"

class ENGINE_API alignas(16) FTransform {
public:
    // ── 默认：零位移，单位四元数，一缩放 ──────────────────
    FTransform();

    FTransform(const FTransform& trans);

    /**
     * @brief 从列主序 4×4 矩阵的线性数组构造。
     *        dat 必须至少包含 16 个元素（column-major 布局）。
     *
     * @param dat   指向矩阵数据的指针
     * @param count 元素个数，用于越界断言（Debug 下）
     */
    template<typename T>
    FTransform(const T* dat, size_t count) {
        ASSERT(count == 16);   // 调试期越界保护，替换为项目实际断言宏

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

        SetQuaternion(MatrixToQuat(Rotation));
    }

    explicit FTransform(const Vector3& position);
    explicit FTransform(const Quaternion& rotation);
    FTransform(const Vector3& position, const Quaternion& rotation);
    FTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);

public:
    // ── 位置 ───────────────────────────────────────────────
    void           SetLocation(Vector3 pos) { vPosition = pos;  bIsDirty = true; }
    const Vector3& GetLocation()        const { return vPosition; }

    // ── 缩放 ───────────────────────────────────────────────
    void           SetScale(Vector3 scale) { vScale = scale;   bIsDirty = true; }
    const Vector3& GetScale()           const { return vScale; }

    // ── 旋转 ───────────────────────────────────────────────
	void              SetRotation(const Vector3& rot) { vRotation = Quaternion(rot);  bIsDirty = true; }
    void              SetQuaternion(const Quaternion& quat) { vRotation = quat; bIsDirty = true; }
    const Quaternion& GetQuaternion()          const { return vRotation; }

    // ── 增量操作 ───────────────────────────────────────────
    void Translate(const Vector3& translation);
    void Rotate(const Quaternion& rotation);
    void Scale(const Vector3& scale);

    // ── 批量设置 ───────────────────────────────────────────
    void SetPR(const Vector3& pos, const Quaternion& rotation);
    void SetPRS(const Vector3& pos, const Quaternion& rotation, const Vector3& scale);
    void TransformRotate(const Vector3& translation, const Quaternion& rotation);

    // ── 矩阵 ───────────────────────────────────────────────
    Matrix4 GetLocal()              const;
    Matrix4 GetWorldMatrix()        const;
    Matrix4 GetInverseWorldMatrix() const;

    // ── 空间变换 ───────────────────────────────────────────
    Vector3 TransformPoint(const Vector3& point)     const;
    Vector3 TransformDirection(const Vector3& direction) const;
    Vector3 InverseTransformPoint(const Vector3& point)     const;

    // ── 脏标志 ─────────────────────────────────────────────
    bool IsDirty()       const { return bIsDirty; }
    void SetDirty() { bIsDirty = true; }
    void UpdateLocal()   const;

private:
    Vector3    vPosition;
    Quaternion vRotation;
    Vector3    vScale;

    mutable bool    bIsDirty;
    mutable Matrix4 Local;
    mutable Matrix4 InverseLocal;
    mutable bool    bInverseDirty;
};