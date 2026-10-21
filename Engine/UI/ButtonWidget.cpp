#include "ButtonWidget.h"

#include "UCanvasRenderer.h"

void UButtonWidget::OnDraw(UCanvasRenderer* InCanvasRenderer) {
	if (!bIsVisible || !InCanvasRenderer) {
		return;
	}

	// 背景：选中时绘制高亮底色；未选中保持透明（仅文字，面板底色透出）
	if (bSelected) {
		UIDrawCallElement Elem;
		// [DIAG-TEST] 强制 y=500 且红色，验证绘制路径是否忠实于顶点数据
		Elem.PositionX = PositionX;
		Elem.PositionY = PositionY;
		Elem.Width = Width;
		Elem.Height = Height;
		Elem.Color = SelectedColor;
		InCanvasRenderer->PushElement(Elem);
	}

	// 文字：垂直居中（按字体行高近似居中），左侧留 4px 内边距
	if (Font && Text.Length() > 0) {
		const float LineHeight = (float)Font->GetLineHeight();
		float TextY = PositionY;
		if (LineHeight > 0.0f && Height > LineHeight) {
			TextY = PositionY + (Height - LineHeight) * 0.5f;
		}
		InCanvasRenderer->PushText(Text, PositionX + 4.0f, TextY, Font, TextColor);
	}

	// 递归绘制子控件
	for (auto* Child : Children) {
		Child->OnDraw(InCanvasRenderer);
	}
}
