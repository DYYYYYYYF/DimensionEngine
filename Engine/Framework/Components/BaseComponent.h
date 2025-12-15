#pragma once

#include "IComponent.h"
#include <string>

class Actor;

class BaseComponent : public IComponent {
public:
	BaseComponent() = default;
	BaseComponent(Actor* Owner, const std::string& Name) : Name_(Name) { SetOwner(Owner); }

	virtual void OnAttach() override {}
	virtual void OnDetach() override {}
	virtual void OnEnable() override {}
	virtual void OnDisable() override {}

	virtual void Tick(float DeltaTime) { (void)DeltaTime; };

	Actor* GetOwner() const { return Owner_; }
	void SetOwner(Actor* Owner) { Owner_ = Owner; }

	bool IsEnabled() const { return IsEnabled_; }
	void SetEnabled(bool Enabled) {
		if (IsEnabled_ != Enabled) {
			IsEnabled_ = Enabled;
			Enabled ? OnEnable() : OnDisable();
		}
	}

protected:
	std::string Name_;
	bool IsEnabled_ = true;

private:
	Actor* Owner_ = nullptr;

};