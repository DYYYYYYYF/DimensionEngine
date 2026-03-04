#pragma once

#include "IComponent.h"
#include <string>

class AActor;

class UBaseComponent : public IComponent {
public:
	UBaseComponent() = default;
	UBaseComponent(AActor* Owner, const std::string& Name) : Name_(Name) { SetOwner(Owner); }

	virtual void OnAttach() override {}
	virtual void OnDetach() override {}
	virtual void OnEnable() override {}
	virtual void OnDisable() override {}

	virtual void Tick(float DeltaTime) { (void)DeltaTime; };

	AActor* GetOwner() const { return Owner_; }
	void SetOwner(AActor* Owner) { Owner_ = Owner; }

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
	AActor* Owner_ = nullptr;

};