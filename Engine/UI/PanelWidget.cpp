#include "PanelWidget.h"

void UPanelWidget::OnMeasure(float ParentWidth, float ParentHeight) {
	Width = DesiredWidth > 0.0f ? DesiredWidth : ParentWidth;
	Height = DesiredHeight > 0.0f ? DesiredHeight : ParentHeight;

	for (auto* Child : Children) {
		Child->OnMeasure(Width, Height);
	}
}

void UPanelWidget::OnLayout(float Left, float Top, float Right, float Bottom) {
	UWidget::OnLayout(Left, Top, Right, Bottom);

	// 默认：子控件保持自身 Position/Size（绝对屏幕坐标），不自动重排
	for (auto* Child : Children) {
		Child->OnLayout(
			Child->GetPositionX(),
			Child->GetPositionY(),
			Child->GetPositionX() + Child->GetWidth(),
			Child->GetPositionY() + Child->GetHeight());
	}
}

void UVerticalBoxPanel::OnMeasure(float ParentWidth, float ParentHeight) {
	Width = DesiredWidth > 0.0f ? DesiredWidth : ParentWidth;
	Height = DesiredHeight > 0.0f ? DesiredHeight : ParentHeight;

	for (auto* Child : Children) {
		Child->OnMeasure(Width, Height);
	}
}

void UVerticalBoxPanel::OnLayout(float Left, float Top, float Right, float Bottom) {
	UWidget::OnLayout(Left, Top, Right, Bottom);

	// 垂直排布：每个子控件使用其测量尺寸，X 方向按左边距对齐，Y 方向依次累加
	float CursorY = Top + PaddingTop;
	for (auto* Child : Children) {
		const float ChildW = Child->GetWidth();
		const float ChildH = Child->GetHeight();
		const float ChildX = Left + PaddingLeft;

		Child->OnLayout(ChildX, CursorY, ChildX + ChildW, CursorY + ChildH);
		CursorY += ChildH + Spacing;
	}
}
