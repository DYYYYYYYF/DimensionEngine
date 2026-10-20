#pragma once

#include "UIDrawCallElement.h"
#include "Containers/TArray.hpp"
#include "Rendering/Resources/Font/Font.hpp"

/**
 * 屏幕空间 UI 画布渲染器：
 * 负责累积一帧内所有 Widget 推入的绘制元素，并交由 UUIWidgetComponent
 * 转换为几何 + 材质，最终经 FUIRenderProxy 由 RenderViewUI 提交渲染。
 * 只做指令累积，不直接接触底层图形 API。
 */
class ENGINE_API UCanvasRenderer {
public:
	UCanvasRenderer() = default;

	// 开始一帧 UI 记录（指定视口尺寸，供元素坐标使用与校验）
	void Begin(uint32_t InViewportWidth, uint32_t InViewportHeight);
	// 结束一帧 UI 记录
	void End();

	// 清空累积的绘制元素
	void Clear();

	// 推入一个绘制元素（宽高非正时忽略）
	void PushElement(const UIDrawCallElement& Element);

	// 推入一段文字：按字体 Atlas 将文本逐字形转换为四边形绘制元素。
	// X/Y 为文字绘制区域左上角（屏幕坐标，Y 向下）；文字颜色经顶点色调制。
	void PushText(const FString& Text, float X, float Y, IFont* Font, const Vector4& Color);

	const TArray<UIDrawCallElement>& GetElements() const { return Elements; }
	uint32_t GetViewportWidth() const { return ViewportWidth; }
	uint32_t GetViewportHeight() const { return ViewportHeight; }

private:
	TArray<UIDrawCallElement> Elements;
	uint32_t ViewportWidth = 0;
	uint32_t ViewportHeight = 0;
	bool bIsRendering = false;
};
