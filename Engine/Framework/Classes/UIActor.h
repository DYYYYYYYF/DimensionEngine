#pragma once

#include "Actor.h"
#include "Framework/Components/UIWidgetComponent.h"

class UWidget;

/**
 * UI Actor：承载一颗 Widget 树并挂载 UUIWidgetComponent。
 * 组件每帧驱动 Widget 树 测量/布局/绘制 生命周期并提交渲染。
 */
class ENGINE_API AUIActor : public AActor {
public:
	DECLARE_CLASS_TYPE(AUIActor)

public:
	AUIActor(const FString& Name);
	virtual ~AUIActor();

	UUIWidgetComponent* GetUIWidgetComponent() const { return UIWidgetComponent; }

	// 设置根 Widget（组件接管所有权）
	void SetRootWidget(UWidget* InRootWidget);

protected:
	UUIWidgetComponent* UIWidgetComponent = nullptr;
};
