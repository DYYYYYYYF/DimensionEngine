#pragma once
#include "Widget.h"

class UCanvasRenderer;

/**
 * UE 风格 ScrollBox 滚动列表容器：
 * - 子项按固定行步长垂直排布（第 i 项位于 ContentStartY + i*RowStep - ScrollOffset）；
 * - 维护滚动偏移（ScrollBy/SetScrollOffset），自动约束在 [0, MaxScrollOffset]；
 * - OnDraw 只绘制落在自身可视矩形内的子项（裁剪，不绘制面板外内容）。
 * 子项通常是 UButtonWidget（列表项）。
 */
class ENGINE_API UScrollBoxWidget : public UWidget {
public:
	UScrollBoxWidget() {
		DesiredWidth = -1.0f;
		DesiredHeight = -1.0f;
	}

	// ── 列表布局参数 ──────────────────────
	// 内容起始 Y（首个子项顶部相对本控件左上角，用于让出标题区）
	void SetContentStartY(float InY) { ContentStartY = InY; }
	// 行步长（每行占位 = 行高 + 行间距）
	void SetRowStep(float InRowStep) { RowStep = InRowStep; }
	// 行高
	void SetRowHeight(float InRowHeight) { RowHeight = InRowHeight; }
	// 内容左右边距（子项 x = PositionX + ContentMarginX，宽 = 本控件宽 - 2*边距）
	void SetContentMarginX(float InMarginX) { ContentMarginX = InMarginX; }

	// ── 滚动 ──────────────────────────────
	void SetScrollOffset(float InOffset) { ScrollOffset = InOffset; ClampScrollOffset(); }
	float GetScrollOffset() const { return ScrollOffset; }
	// Delta > 0 向下滚动（显示靠后的行），Delta < 0 向上滚动
	void ScrollBy(float Delta) { ScrollOffset += Delta; ClampScrollOffset(); }
	float GetMaxScrollOffset() const { return MaxScrollOffset; }

	// 内容数量（影响滚动范围）
	void SetContentCount(int InCount) { ContentCount = InCount; }
	int GetContentCount() const { return ContentCount; }

	// 按当前 ScrollOffset/布局参数重新排布子项位置（滚动或内容变化后调用）
	void LayoutChildren();

	virtual void OnLayout(float Left, float Top, float Right, float Bottom) override;
	virtual void OnDraw(class UCanvasRenderer* InCanvasRenderer) override;
	// 命中分发与 OnDraw 同裁剪判据：仅当点击落在本控件可视矩形内时
	// 才向子项分发，避免可视区外的按钮（滚出面板但仍挂在树上的行）
	// 被误命中，导致点击行与实际高亮行错位。
	virtual bool DispatchMouseClick(float X, float Y) override;

private:
	void ClampScrollOffset() {
		if (MaxScrollOffset < 0.0f) {
			MaxScrollOffset = 0.0f;
		}
		if (ScrollOffset > MaxScrollOffset) {
			ScrollOffset = MaxScrollOffset;
		}
		if (ScrollOffset < 0.0f) {
			ScrollOffset = 0.0f;
		}
	}

private:
	float ContentStartY = 30.0f;
	float RowStep = 28.0f;
	float RowHeight = 26.0f;
	float ContentMarginX = 2.0f;
	float ScrollOffset = 0.0f;
	float MaxScrollOffset = 0.0f;
	int ContentCount = 0;
};
