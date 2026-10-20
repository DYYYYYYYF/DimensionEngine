#include "ImageWidget.h"

#include "UI/UIDrawCallElement.h"
#include "UI/UCanvasRenderer.h"
#include "Systems/TextureSystem.h"

void UImageWidget::SetTexture(const FString& Path) {
	// 通过资源系统加载纹理：TextureSystem 内部经 ResourceSystem 读取磁盘像素，
	// 并创建底层图形 API 纹理资源（MaterialSystem 加载材质贴图同路径）。
	Texture = TextureSystem::Get().Acquire(Path);
	if (!Texture) {
		GLOG(Log::eWarn, "UImageWidget::SetTexture: Failed to load texture '%s'.", Path.CStr());
		return;
	}
}

void UImageWidget::OnMeasure(float ParentWidth, float ParentHeight) {
	Width = DesiredWidth > 0.0f ? DesiredWidth : 100.0f;
	Height = DesiredHeight > 0.0f ? DesiredHeight : 100.0f;

	for (auto* Child : Children) {
		Child->OnMeasure(Width, Height);
	}
}

void UImageWidget::OnDraw(class UCanvasRenderer* InCanvasRenderer) {
	if (!bIsVisible || !InCanvasRenderer) return;

	// 构造专属于 UI 的 2D 渲染命令（传递屏幕坐标、宽高、UV、颜色、纹理句柄）
	UIDrawCallElement Element;
	Element.PositionX = this->PositionX;
	Element.PositionY = this->PositionY;
	Element.Width = this->Width;
	Element.Height = this->Height;
	Element.UV0 = this->UV0;
	Element.UV1 = this->UV1;
	Element.Color = this->Color;
	Element.Texture = this->Texture;

	// 提交给专用的 UI 渲染器
	InCanvasRenderer->PushElement(Element);

	// 调用基类绘制可能存在的子控件（比如Image控件里叠个文字）
	UWidget::OnDraw(InCanvasRenderer);
}
