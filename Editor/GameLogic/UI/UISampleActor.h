#pragma once

#include "Framework/Classes/UIActor.h"

/**
 * UI 系统演示 Actor：
 * 创建一颗 Widget 树（垂直盒 + 若干图片/纯色矩形控件），
 * 由 UUIWidgetComponent 每帧驱动 测量/布局/绘制 并提交渲染。
 */
class UISampleActor : public AUIActor {
public:
	UISampleActor(const FString& Name);
	virtual ~UISampleActor() = default;

private:
	void BuildWidgetTree();
};
