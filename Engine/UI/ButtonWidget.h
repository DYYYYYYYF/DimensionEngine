#pragma once
#include "Widget.h"
#include "Rendering/Resources/Font/Font.hpp"
#include <functional>

class UCanvasRenderer;

/**
 * UE 风格按钮控件：
 * - 可显示文字（SetText + 字体 Atlas，通过 UCanvasRenderer::PushText 绘制）；
 * - 支持 OnClick 回调（点击命中后触发，事件被消费不再冒泡）；
 * - 支持选中态（SetSelected → 高亮底色），用于列表行选中高亮。
 * 默认背景透明（仅文字）；选中/悬停时绘制背景矩形。
 */
class ENGINE_API UButtonWidget : public UWidget {
public:
	using OnClickHandler = std::function<void(UButtonWidget*)>;

	UButtonWidget() {
		DesiredWidth = 100.0f;
		DesiredHeight = 26.0f;
	}

	// ── 文字 ──────────────────────────────
	void SetText(const FString& InText) { Text = InText; }
	const FString& GetText() const { return Text; }

	void SetFont(IFont* InFont) { Font = InFont; }
	IFont* GetFont() const { return Font; }

	void SetTextColor(const Vector4& InColor) { TextColor = InColor; }
	const Vector4& GetTextColor() const { return TextColor; }

	// ── 交互 ──────────────────────────────
	void SetOnClick(OnClickHandler InHandler) { OnClickHandler_ = InHandler; }
	const OnClickHandler& GetOnClick() const { return OnClickHandler_; }

	// 选中态（由 OnClick 回调/外部刷新维护，驱动高亮底色）
	void SetSelected(bool bInSelected) { bSelected = bInSelected; }
	bool IsSelected() const { return bSelected; }

	// 悬停底色（预留，未启用）
	void SetHoverColor(const Vector4& InColor) { HoverColor = InColor; }

	// 选中高亮底色（默认 UE 蓝色调，与旧 SelectionBar 观感一致）
	void SetSelectedColor(const Vector4& InColor) { SelectedColor = InColor; }

	// 点击命中后触发回调并消费事件
	virtual bool OnMouseClick(float X, float Y) override {
		if (OnClickHandler_) {
			OnClickHandler_(this);
		}
		return true;
	}

	// 绘制：选中/悬停时先画背景矩形，再画文字
	virtual void OnDraw(class UCanvasRenderer* CanvasRenderer) override;

private:
	FString Text;
	IFont* Font = nullptr;
	Vector4 TextColor = Vector4(0.82f, 0.84f, 0.88f, 1.0f);
	Vector4 HoverColor = Vector4(0.16f, 0.18f, 0.24f, 1.0f);
	Vector4 SelectedColor = Vector4(0.22f, 0.45f, 0.90f, 0.85f);
	bool bSelected = false;
	OnClickHandler OnClickHandler_;
};
