#pragma once
#include "Widget.h"
#include "Rendering/Resources/Texture/Texture.hpp"

class UCanvasRenderer;

/**
 * 图片/纯色矩形控件：
 * - 设置纹理后显示图片；
 * - 未设置纹理时绑定白色纹理 + Color，可绘制任意纯色矩形。
 */
class ENGINE_API UImageWidget : public UWidget {
public:
	UImageWidget() { DesiredWidth = 100.0f; DesiredHeight = 100.0f; }

	// 直接绑定已加载纹理
	void SetTexture(UTexture* InTexture) { Texture = InTexture; }

	// 通过资源系统按名称加载纹理（如 "DimensionEngine"）
	void SetTexture(const FString& Path);

	// 设置 UV 范围（默认整张纹理 0,0 -> 1,1）
	void SetUV(float U0, float V0, float U1, float V1) { UV0 = Vector2(U0, V0); UV1 = Vector2(U1, V1); }

	// 测量：尺寸 = DesiredSize（未设置时使用默认 100x100）
	virtual void OnMeasure(float ParentWidth, float ParentHeight) override;

	// 绘制：推入绘制元素后递归绘制子控件
	virtual void OnDraw(class UCanvasRenderer* InCanvasRenderer) override;

private:
	UTexture* Texture = nullptr;
	Vector2 UV0 = Vector2(0.0f, 0.0f);
	Vector2 UV1 = Vector2(1.0f, 1.0f);
};
