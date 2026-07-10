#pragma once
#include "Core/CoreMinimal.hpp"
#include "Framework/BaseObject.h"
#include "Containers/TArray.hpp"

class UWidget : public ABaseObject {
public:
	UWidget() : Parent(nullptr), bIsVisible(true) {}
	virtual ~UWidget() { ClearChildren(); }

	// 树状管理
	void AddChild(UWidget* Child) {
		if (Child) {
			Child->Parent = this;
			Children.Push(Child);
		}
	}

	void ClearChildren() {
		for (auto* Child : Children) { delete Child; }
		Children.Clear();
	}

	// UI核心生命周期：测量 -> 布局 -> 绘制
	virtual void OnMeasure(float ParentWidth, float ParentHeight) {} // 确定自己多大
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) {} // 确定自己在哪
	virtual void OnDraw(class UCanvasRenderer* CanvasRenderer) {
		if (!bIsVisible) return;
		// 递归绘制子控件
		for (auto* Child : Children) {
			Child->OnDraw(CanvasRenderer);
		}
	}

protected:
	UWidget* Parent = nullptr;
	TArray<UWidget*> Children;

	// UI 局部变换数据（屏幕坐标或相对坐标）
	float PositionX = 0.0f, PositionY = 0.0f;
	float Width = 0.0f, Height = 0.0f;
	bool bIsVisible;
};