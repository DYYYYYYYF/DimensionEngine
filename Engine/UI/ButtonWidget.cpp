#include "ButtonWidget.h"

#include "UCanvasRenderer.h"

void UButtonWidget::OnDraw(UCanvasRenderer* CanvasRenderer) {
	if (!bIsVisible || !CanvasRenderer) {
		return;
	}

	// 背景：选中时绘制高亮底色；未选中保持透明（仅文字，面板底色透出）
	if (bSelected) {
		UIDrawCallElement Elem;
		Elem.PositionX = PositionX;
		Elem.PositionY = PositionY;
		Elem.Width = Width;
		Elem.Height = Height;
		Elem.Color = SelectedColor;
		CanvasRenderer->PushElement(Elem);
	}

	// 文字：垂直居中（按字体行高近似居中），左侧留 4px 内边距
	if (Font && Text.Length() > 0) {
		const float LineHeight = (float)Font->GetLineHeight();
		float TextY = PositionY;
		if (LineHeight > 0.0f && Height > LineHeight) {
			TextY = PositionY + (Height - LineHeight) * 0.5f;
		}
		CanvasRenderer->PushText(Text, PositionX + 4.0f, TextY, Font, TextColor);
	}

	// 递归绘制子控件
	for (auto* Child : Children) {
		Child->OnDraw(CanvasRenderer);
	}
}
