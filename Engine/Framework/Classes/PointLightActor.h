#pragma once

#include "Actor.h"

class UPointLightComponent;
class JsonObject;

/**
 * @brief 点光源 Actor。
 *
 * 场景中点光源的统一配置入口：持有一个点光源组件，构造时注册进 LightSystem，
 * 销毁时注销，渲染通道从 LightSystem 取全部点光源并打包上传。
 * 光源位置即 Actor（组件）的世界位置，可直接 SetActorLocation 或经配置的 Position 字段调整。
 *
 * 序列化：与 Editor/Config.json 的 PointLights 数组元素（一个对象即一盏灯）互转，
 * 字段为 Position(Vector3) / Color(Vector4) / Intensity(float) / Attenuation(Vector3) / Enabled(bool)，
 * 缺失字段保留组件默认值。
 */
class ENGINE_API APointLightActor : public AActor {
	DECLARE_CLASS_TYPE(APointLightActor)

public:
	APointLightActor(const FString& Name);
	virtual ~APointLightActor() override;

	UPointLightComponent* GetPointLightComponent() const { return PointLightComponent; }

	/** 从 JSON 配置读取点光源参数并应用到组件；缺失字段保留组件当前值，无组件时返回 false */
	bool LoadFromConfig(const JsonObject& Config);
	/** 把组件当前的点光源参数写回 JSON 配置对象；无组件时直接返回 */
	void SaveToConfig(JsonObject& Config) const;

protected:
	UPointLightComponent* PointLightComponent;
};
