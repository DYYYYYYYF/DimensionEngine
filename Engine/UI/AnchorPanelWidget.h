#pragma once
#include "Widget.h"

class UCanvasRenderer;

/**
 * UE 风格锚点面板容器：
 * 通过 AnchorMin/AnchorMax（父矩形归一化 0..1）与像素 Offsets 计算自身矩形。
 * 典型用法：锚定屏幕左侧 —— AnchorMin=(0,0) AnchorMax=(0,1)，
 * Offsets=(Left, Top, Right, Bottom) 中 Right-Bottom 决定宽度/高度。
 * OnLayout 先按锚点算出自身矩形，再按自身矩形递归布局全部子控件（铺满）。
 */
class ENGINE_API UAnchorPanelWidget : public UWidget {
public:
	UAnchorPanelWidget() {
		DesiredWidth = -1.0f;
		DesiredHeight = -1.0f;
	}

	// UE 语义：AnchorMin/AnchorMax 为父矩形归一化坐标（0..1）
	void SetAnchors(const Vector2& InAnchorMin, const Vector2& InAnchorMax) {
		AnchorMin = InAnchorMin;
		AnchorMax = InAnchorMax;
	}
	const Vector2& GetAnchorMin() const { return AnchorMin; }
	const Vector2& GetAnchorMax() const { return AnchorMax; }

	// Offsets：相对锚点的像素边距（Left/Top 相对 AnchorMin，Right/Bottom 相对 AnchorMax）
	void SetOffsets(float InLeft, float InTop, float InRight, float InBottom) {
		OffsetLeft = InLeft;
		OffsetTop = InTop;
		OffsetRight = InRight;
		OffsetBottom = InBottom;
	}

	virtual void OnMeasure(float ParentWidth, float ParentHeight) override;
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) override;

private:
	Vector2 AnchorMin = Vector2(0.0f, 0.0f);
	Vector2 AnchorMax = Vector2(0.0f, 0.0f);
	float OffsetLeft = 0.0f;
	float OffsetTop = 0.0f;
	float OffsetRight = 0.0f;
	float OffsetBottom = 0.0f;
};
