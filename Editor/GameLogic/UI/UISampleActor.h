#pragma once

#include "Framework/Classes/Actor.h"

class UPanelWidget;

/**
 * UI 系统演示 Actor：
 * 创建一颗 Widget 树（垂直盒 + 若干图片/纯色矩形控件），
 * Widget 树由 UWorld 每帧驱动 测量/布局/绘制 并提交渲染。
 */
class UISampleActor : public AActor {
public:
	UISampleActor(const FString& Name);
	virtual ~UISampleActor();

private:
	void BuildWidgetTree();

private:
	UPanelWidget* Root = nullptr;
};
