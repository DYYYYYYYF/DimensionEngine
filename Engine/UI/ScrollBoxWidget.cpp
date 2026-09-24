#include "ScrollBoxWidget.h"

#include "UCanvasRenderer.h"

void UScrollBoxWidget::OnLayout(float Left, float Top, float Right, float Bottom) {
	PositionX = Left;
	PositionY = Top + ContentStartY;
	Width = Right - Left;
	Height = Bottom - Top - ContentStartY;
	if (Height < 0.0f) {
		Height = 0.0f;
	}

	// 滚动范围 = 内容总高 - 可视高（可视区 = 本控件整个矩形）
	const float ViewH = Height;
	const float ContentH = (float)ContentCount * RowStep;
	MaxScrollOffset = ContentH - ViewH;
	if (MaxScrollOffset < 0.0f) {
		MaxScrollOffset = 0.0f;
	}
	ClampScrollOffset();

	LayoutChildren();
}

void UScrollBoxWidget::LayoutChildren() {
	for (size_t i = 0; i < Children.Size(); ++i) {
		UWidget* Child = Children[i];
		if (!Child) {
			continue;
		}
		const float ChildY = PositionY + (float)i * RowStep - ScrollOffset;
		Child->SetPosition(PositionX + ContentMarginX, ChildY);
		if (Child->GetDesiredHeight() <= 0.0f) {
			Child->SetDesiredSize(Child->GetDesiredWidth(), RowHeight);
		}
		// 子项宽度 = 本控件宽 - 左右边距（子项测量时按父宽填充）
		Child->OnMeasure(Width - ContentMarginX * 2.0f, RowHeight);
	}
}

void UScrollBoxWidget::OnDraw(UCanvasRenderer* CanvasRenderer) {
	if (!bIsVisible || !CanvasRenderer) {
		return;
	}

	// 绘制前先布局子项：按钮池可能在上一帧绘制之后才 AddChild / 更新滚动偏移，
	// 若沿用旧位置会画出错位内容（新增按钮曾画在屏幕原点）。此处保证每帧
	// 子项位置/尺寸始终与最新滚动状态一致（与 OnLayout 中调用幂等）。
	LayoutChildren();

	// 可视区（裁剪窗口）：本控件矩形
	const float ViewTop = PositionY;
	const float ViewBottom = PositionY + Height;

	// 只绘制与可视区相交的子项（滚动裁剪，面板外内容不绘制）
	for (auto* Child : Children) {
		if (!Child || !Child->IsVisible()) {
			continue;
		}
		const float ChildTop = Child->GetPositionY();
		const float ChildBottom = ChildTop + Child->GetHeight();
		if (ChildBottom < ViewTop || ChildTop > ViewBottom) {
			continue;
		}
		Child->OnDraw(CanvasRenderer);
	}
}

bool UScrollBoxWidget::DispatchMouseClick(float X, float Y) {
	if (!bIsVisible) {
		return false;
	}
	// 与 OnDraw 相同的可视区裁剪判据：点击必须在滚动可视矩形内，
	// 否则即使命中某个"滚出可视区"的按钮（仍挂在树上）也不应响应。
	if (X < PositionX || X > PositionX + Width ||
		Y < PositionY || Y > PositionY + Height) {
		return false;
	}
	// 子项优先：命中子按钮即由按钮 OnClick 消费事件。
	for (auto* Child : Children) {
		if (!Child || !Child->IsVisible()) {
			continue;
		}
		if (Child->DispatchMouseClick(X, Y)) {
			return true;
		}
	}
	return false;
}
