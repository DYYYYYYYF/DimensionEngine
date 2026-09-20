#pragma once

#include "UIDrawCallElement.h"
#include "Containers/TArray.hpp"

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
	void Begin(uint32_t ViewportWidth, uint32_t ViewportHeight);
	// 结束一帧 UI 记录
	void End();

	// 清空累积的绘制元素
	void Clear();

	// 推入一个绘制元素（宽高非正时忽略）
	void PushElement(const UIDrawCallElement& Element);

	const TArray<UIDrawCallElement>& GetElements() const { return Elements; }
	uint32_t GetViewportWidth() const { return ViewportWidth; }
	uint32_t GetViewportHeight() const { return ViewportHeight; }

private:
	TArray<UIDrawCallElement> Elements;
	uint32_t ViewportWidth = 0;
	uint32_t ViewportHeight = 0;
	bool bIsRendering = false;
};
