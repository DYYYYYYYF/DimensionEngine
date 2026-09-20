#pragma once

#include "Math/MathTypes.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

/**
 * 一次 UI 绘制指令：屏幕空间四边形（左上角 XY + 宽高），带 UV、颜色与可选纹理。
 * 由 UWidget::OnDraw 推入 UCanvasRenderer，最终由 UUIWidgetComponent 转换为
 * Vertex2D 几何并经 FUIRenderProxy 由 RenderViewUI 提交（Shader.Builtin.UI）。
 */
struct ENGINE_API UIDrawCallElement {
	// 屏幕坐标（四边形左上角），原点在窗口左上角，Y 向下
	float PositionX = 0.0f;
	float PositionY = 0.0f;

	// 四边形宽高（像素）
	float Width = 0.0f;
	float Height = 0.0f;

	// UV 范围（默认整张纹理）
	Vector2 UV0 = Vector2(0.0f, 0.0f);
	Vector2 UV1 = Vector2(1.0f, 1.0f);

	// 顶点颜色（与纹理采样结果相乘；纯色矩形可绑定白色纹理）
	Vector4 Color = Vector4(1.0f);

	// 绑定的漫反射纹理；nullptr 表示使用纯白色纹理（纯色矩形）
	UTexture* Texture = nullptr;
};
