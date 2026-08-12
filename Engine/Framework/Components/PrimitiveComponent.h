#pragma once

#include "SceneComponent.h"

class FRenderProxy;

/**
 * 1. 未来可能需要多继承自物理组件
 * 2. 主要存放一些配置相关信息
 */
class DAPI UPrimitiveComponent : public USceneComponent {
public:
	DECLARE_CLASS_TYPE(UPrimitiveComponent)

public:
	UPrimitiveComponent(const FString& Name) : USceneComponent(Name) {}
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void OnTransformChanged() override;
	virtual bool CreateRenderProxy() = 0;

protected:
	virtual void UpdateRenderProxy() {};

protected:
	bool IsRegistered = false;
	FRenderProxy* RenderProxy = nullptr;

};
