#include "PointLightComponent.h"
#include "Core/EngineLogger.hpp"

UPointLightComponent::UPointLightComponent(const FString& Name) : USceneComponent(Name) {
	// 默认值与引擎此前的硬编码点光源参数保持一致：白色点光，衰减 1/(1 + 0.05d + 0.003d^2)
	Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	Intensity = 1.0f;
	Attenuation = Vector3(1.0f, 0.05f, 0.003f);
	bEnabled = true;
}

void UPointLightComponent::SetIntensity(float InIntensity) {
	// 负强度会让点光源变成"减光"，此处钳制到非负
	if (InIntensity < 0.0f) {
		GLOG(Log::eWarn, "PointLightComponent::SetIntensity: negative intensity %.3f is clamped to 0.", InIntensity);
		Intensity = 0.0f;
		return;
	}

	Intensity = InIntensity;
}

void UPointLightComponent::SetAttenuation(const Vector3& InAttenuation) {
	// constant 必须为正，否则距离 0 处出现除零（衰减趋于无穷）
	if (InAttenuation.x <= 0.0f) {
		GLOG(Log::eWarn, "PointLightComponent::SetAttenuation: non-positive constant %.3f is ignored.", InAttenuation.x);
		return;
	}

	Attenuation = InAttenuation;
}

Vector3 UPointLightComponent::GetWorldPosition() const {
	// 世界位置 = 组件世界变换对原点的变换结果
	return TransformPointToWorld(Vector3(0.0f, 0.0f, 0.0f));
}

Vector4 UPointLightComponent::GetAttenuationVector() const {
	return Vector4(Attenuation.x, Attenuation.y, Attenuation.z, Intensity);
}
