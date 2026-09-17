#pragma once

#include "Defines.hpp"
#include "Containers/TArray.hpp"

class UDirectionalLightComponent;
class UPointLightComponent;

/**
 * @brief 光照系统。
 *
 * 维护场景中已注册的方向光与点光源组件，为渲染通道提供统一的取用入口：
 *  - 方向光：提供"当前主光源"（阴影通道与延迟光照的方向光项都用它）；
 *  - 点光源：提供整个列表，延迟光照通道把列表打包成数组 Uniform 上传，
 *    着色器按数量循环累加，因此不再存在任何硬编码光源。
 * 组件在挂载/卸载时自行注册与注销，因此不存在渲染侧持有失效指针的情况。
 */
class ENGINE_API LightSystem {
public:
	static LightSystem& Get();

	/** 组件挂载时注册；若当前尚无主光源则同时置为主光源 */
	void RegisterLight(UDirectionalLightComponent* Light);
	/** 组件卸载时注销；若被注销者正是主光源则自动切换到剩余的第一盏，没有则置空 */
	void UnregisterLight(UDirectionalLightComponent* Light);

	/** 取当前主方向光（无光源时返回 nullptr，调用方需回退到默认值） */
	UDirectionalLightComponent* GetMainLight() const { return MainLight; }
	/** 已注册的方向光数量 */
	size_t GetLightCount() const { return Lights.Size(); }

	/** 点光源组件挂载时注册（重复注册会被忽略） */
	void RegisterPointLight(UPointLightComponent* Light);
	/** 点光源组件卸载时注销 */
	void UnregisterPointLight(UPointLightComponent* Light);
	/**
	 * 已注册的点光源列表（按注册顺序）。
	 * 上限由渲染侧的上传数组长度（FRAME_DATA_MAX_POINT_LIGHTS）决定，超出部分不参与渲染。
	 */
	const TArray<UPointLightComponent*>& GetPointLights() const { return PointLights; }
	/** 已注册的点光源数量 */
	size_t GetPointLightCount() const { return PointLights.Size(); }

private:
	LightSystem() = default;

	TArray<UDirectionalLightComponent*> Lights;
	UDirectionalLightComponent* MainLight = nullptr;

	TArray<UPointLightComponent*> PointLights;
};
