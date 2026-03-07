#pragma once

#include "Component.h"
#include "Math/Transform.h"

/**
 * @brief Transform 组件。
 *
 * 多重继承 UBaseComponent（组件身份）和 FTransform（变换数据与接口）。
 * 不重复任何 FTransform 方法——所有位置、旋转、缩放、矩阵操作
 * 直接通过 FTransform 的公开接口使用。
 * 
 */
class ENGINE_API alignas(16) UTransformComponent : public UComponent, public FTransform {
public:
    DECLARE_CLASS_TYPE(UTransformComponent)

public:
    // 默认构造：零位移、单位四元数、一缩放
    UTransformComponent() {
		Name_ = "TransformComponent";
    }

    // 从现有 FTransform 构造
    explicit UTransformComponent(const FTransform& trans)
        : FTransform(trans) {
        Name_ = "TransformComponent";
    }

    // 从列主序 4×4 矩阵数组构造，委托给 FTransform 模板构造
    template<typename T>
    explicit UTransformComponent(const T* dat, size_t count)
        : FTransform(dat, count) {
        Name_ = "TransformComponent";
    }
};