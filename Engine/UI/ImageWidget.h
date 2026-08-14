#pragma once
#include "Widget.h"

class UImageWidget : public UWidget {
public:
	UImageWidget() : TextureHandle(nullptr) {}

	void SetTexture(const FString& Path) {
		// 调用 ResourceSystem 加载纹理并记录句柄[cite: 1]
	}

	virtual void OnDraw(class UCanvasRenderer* CanvasRenderer) override {
		if (!bIsVisible || !TextureHandle) return;

		//// 构造专属于 UI 的 2D 渲染命令（传递屏幕坐标、宽高、UV、颜色、纹理句柄）
		//UIDrawCallElement Element;
		//Element.PositionX = this->PositionX;
		//Element.PositionY = this->PositionY;
		//Element.Width = this->Width;
		//Element.Height = this->Height;
		//Element.Texture = this->TextureHandle;
		//Element.Color = FVector4(1, 1, 1, 1); // 支持运行时改Alpha或颜色

		//// 提交给专用的 UI 渲染器
		//CanvasRenderer->PushElement(Element);

		//// 调用基类绘制可能存在的子控件（比如Image控件里叠个文字）
		//UWidget::OnDraw(CanvasRenderer);
	}

private:
	void* TextureHandle = nullptr; // 底层图形API对应的纹理句柄
};