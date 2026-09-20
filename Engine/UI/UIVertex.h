#pragma once

#include "Math/MathTypes.hpp"
#include "Rendering/Resources/Texture/Texture.hpp"

/**
 * UI 合批顶点：屏幕空间位置 + UV + 顶点颜色。
 * 与 Vertex2D（原 UI shader 逐元素材质 uniform 传色）不同，颜色随顶点提交，
 * 使 Batched shader 能以单次 DrawCall 绘制不同颜色的元素，无需每元素
 * 切换材质实例（diffuse_color 恒为白，最终颜色 = 纹理采样 * 顶点色）。
 */
struct ENGINE_API UIVertex {
	Vector2 position = Vector2(0.0f, 0.0f);
	Vector2 texcoord = Vector2(0.0f, 0.0f);
	Vector4 color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	UIVertex() = default;

	UIVertex(const Vector2& InPosition, const Vector2& InTexcoord, const Vector4& InColor)
		: position(InPosition), texcoord(InTexcoord), color(InColor) {}
};
