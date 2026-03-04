#pragma once

#include "Framework/IObject.h"

class AActor;

class IComponent : public IObject {
public:
	virtual ~IComponent() = default;

	// 生命周期
	virtual void OnAttach()  = 0;   // 组件添加到Actor时调用
	virtual void OnDetach()  = 0;   // 组件从Actor移除时调用
	virtual void OnEnable()  = 0;   // 组件启用时调用
	virtual void OnDisable() = 0;   // 组件禁用时调用

	// 更新
	virtual void Tick(float deltaTime) = 0;

	// 获取所属Actor
	virtual AActor* GetOwner() const = 0;
	virtual void SetOwner(AActor* owner) = 0;
	
	// 启用/禁用
	virtual bool IsEnabled() const = 0;
	virtual void SetEnabled(bool enabled) = 0;

};