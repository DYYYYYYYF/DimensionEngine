#pragma once
#include "Widget.h"

/**
 * 容器基类：负责对子控件执行布局算法。
 * - OnMeasure 默认遍历子控件递归测量；
 * - OnLayout 默认记录自身矩形后，按子控件自身 Position/Size 递归布局（不自动重排）。
 */
class ENGINE_API UPanelWidget : public UWidget {
public:
	// 测量：尺寸 = DesiredSize 或父可用空间，并递归测量子控件
	virtual void OnMeasure(float ParentWidth, float ParentHeight) override;

	// 布局：记录自身矩形，并递归将子控件布局到各自位置（默认不做自动排布）
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) override;
};

/**
 * 简单垂直盒布局：子控件按测量高度从上到下依次排布，水平居中。
 * 是"最简单的 UI 系统"中的首个可用布局容器。
 */
class ENGINE_API UVerticalBoxPanel : public UPanelWidget {
public:
	UVerticalBoxPanel() { DesiredWidth = 0.0f; DesiredHeight = 0.0f; }

	void SetSpacing(float InSpacing) { Spacing = InSpacing; }
	float GetSpacing() const { return Spacing; }

	// 内容边距：子控件排布起点相对容器左上角的偏移
	void SetPadding(float InPaddingLeft, float InPaddingTop) { PaddingLeft = InPaddingLeft; PaddingTop = InPaddingTop; }
	float GetPaddingLeft() const { return PaddingLeft; }
	float GetPaddingTop() const { return PaddingTop; }

	virtual void OnMeasure(float ParentWidth, float ParentHeight) override;
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) override;

private:
	float Spacing = 4.0f;
	float PaddingLeft = 0.0f;
	float PaddingTop = 0.0f;
};
