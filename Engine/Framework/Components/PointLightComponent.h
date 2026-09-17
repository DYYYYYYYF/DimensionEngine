#pragma once

#include "Framework/Components/SceneComponent.h"
#include "Math/MathTypes.hpp"

/**
 * @brief 点光源组件。
 *
 * 与方向光组件同属"光源数据"的承载者：颜色、强度、距离衰减参数以及世界位置（取自组件自身变换）。
 * 组件自身不参与渲染调用，渲染通道通过 LightSystem 逐帧取用这些数据并上传 GPU：
 *  - 延迟光照通道（Lighting Pass）把多盏点光源打包成数组 Uniform 上传，
 *    着色器按 point_light_count 循环累加，不再存在任何硬编码光源。
 *
 * 位置说明：点光源位置取组件的世界位置（TransformPointToWorld），
 * 因此移动挂载该组件的 Actor 即可移动光源。
 */
class ENGINE_API UPointLightComponent : public USceneComponent {
	DECLARE_CLASS_TYPE(UPointLightComponent)

public:
	UPointLightComponent(const FString& Name);

	//~ Begin 颜色与强度
	/** 光源颜色：rgb 为线性颜色，a 保留 1.0（亮度由 Intensity 控制） */
	void SetColor(const Vector4& InColor) { Color = InColor; }
	const Vector4& GetColor() const { return Color; }

	/** 强度（标量，非负），上传时乘到颜色 rgb 上，决定局部补光的亮度 */
	void SetIntensity(float InIntensity);
	float GetIntensity() const { return Intensity; }
	//~ End 颜色与强度

	//~ Begin 距离衰减
	/**
	 * 设置距离衰减系数：x = constant，y = linear，z = quadratic，
	 * 对应 1 / (constant + linear * d + quadratic * d * d)。
	 * quadratic 越大光池越小、衰减越快。
	 */
	void SetAttenuation(const Vector3& InAttenuation);
	const Vector3& GetAttenuation() const { return Attenuation; }
	//~ End 距离衰减

	/** 是否参与渲染：关闭后该光源不会被上传（相当于临时熄灭，可随时重新点亮） */
	void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }
	bool IsEnabled() const { return bEnabled; }

	/** 光源世界位置（组件世界变换的原点） */
	Vector3 GetWorldPosition() const;

	/** 上传给 GPU 的衰减向量：(constant, linear, quadratic, 强度) */
	Vector4 GetAttenuationVector() const;

protected:
	// 光源颜色（rgb = 线性颜色，a = 亮度系数）
	Vector4 Color;
	// 强度：乘到颜色 rgb 上
	float Intensity;
	// 距离衰减系数 (constant, linear, quadratic)
	Vector3 Attenuation;
	// 渲染开关
	bool bEnabled;
};
