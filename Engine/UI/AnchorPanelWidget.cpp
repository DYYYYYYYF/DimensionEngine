#include "AnchorPanelWidget.h"

// 测量阶段：父矩形为整个视口（原点 0,0），用锚点公式计算自身矩形。
// 此阶段只负责"算矩形"，真正采用在 OnLayout（幂等），避免多次布局叠加漂移。
void UAnchorPanelWidget::OnMeasure(float ParentWidth, float ParentHeight) {
	// 锚点公式：位置 = 父原点(0,0) + AnchorMin * 父尺寸 + Offset
	//        尺寸 = (AnchorMax - AnchorMin) * 父尺寸 + (OffsetRight - OffsetLeft)
	PositionX = AnchorMin.x * ParentWidth + OffsetLeft;
	PositionY = AnchorMin.y * ParentHeight + OffsetTop;
	Width = (AnchorMax.x - AnchorMin.x) * ParentWidth + (OffsetRight - OffsetLeft);
	Height = (AnchorMax.y - AnchorMin.y) * ParentHeight + (OffsetBottom - OffsetTop);

	// 递归测量子控件：默认铺满本面板矩形
	for (auto* Child : Children) {
		if (!Child) {
			continue;
		}
		Child->OnMeasure(Width, Height);
	}
}

void UAnchorPanelWidget::OnLayout(float Left, float Top, float Right, float Bottom) {
	// 父容器（UPanelWidget）传入的是本控件当前矩形（Left=PositionX, Top=PositionY,
	// Right=PositionX+Width, Bottom=PositionY+Height），直接采用即可。
	// 注意：不能再次叠加锚点偏移（会把位置逐帧放大漂移到屏幕外）。
	PositionX = Left;
	PositionY = Top;
	Width = Right - Left;
	Height = Bottom - Top;

	// 递归布局子控件：默认铺满本面板矩形
	for (auto* Child : Children) {
		if (!Child) {
			continue;
		}
		Child->OnLayout(PositionX, PositionY, PositionX + Width, PositionY + Height);
	}
}

