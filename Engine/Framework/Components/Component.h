#pragma once

#include "Framework/BaseObject.h"
#include "Containers/FString.hpp"

class AActor;

class ENGINE_API UComponent : public ABaseObject {
public:
	DECLARE_CLASS_TYPE(UComponent)

public:
	UComponent() = default;
	UComponent(const FString& Name) : Name_(Name) {};
	UComponent(AActor* Owner, const FString& Name) : Name_(Name) { SetOwner(Owner); }
	virtual ~UComponent() = default;

	virtual void PreInitialize() override {}
	virtual bool Initialize() override { return true; }
	virtual void PostInitialize() override {}

	// 生命周期
	virtual void OnAttach() {};   // 组件添加到Actor时调用
	virtual void OnDetach() {};   // 组件从Actor移除时调用
	virtual void OnEnable() {};   // 组件启用时调用
	virtual void OnDisable() {};   // 组件禁用时调用

	// 更新
	virtual void Tick(float deltaTime) {};

	// 获取所属Actor
	virtual AActor* GetOwner() const { return Owner_; };
	virtual void SetOwner(AActor* owner) { Owner_ = owner; }
	
	// 启用/禁用
	bool IsEnabled() const { return IsEnabled_; }
	void SetEnabled(bool Enabled) {
		if (IsEnabled_ != Enabled) {
			IsEnabled_ = Enabled;
			Enabled ? OnEnable() : OnDisable();
		}
	}

protected:
	FString Name_;
	AActor* Owner_ = nullptr;
	bool IsEnabled_ = true;

};