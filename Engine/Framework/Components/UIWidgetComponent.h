#pragma once

#include "Framework/Components/PrimitiveComponent.h"
#include "UI/UCanvasRenderer.h"

class UWidget;
class UGeometry;
class UTexture;

/**
 * UI 合批分组：同纹理元素合并为一个批次几何（一次 DrawCall）。
 */
struct UIBlendGroup {
	UTexture* Texture = nullptr;
	uint32_t ElementCount = 0;
	// 上次上传的顶点数（用于判断尺寸是否变化：变化时重新上传索引）
	uint32_t LastVertexCount = 0;
};

/**
 * UI 组件：持有 Widget 树，每帧驱动 测量 -> 布局 -> 绘制 生命周期，
 * 把 UCanvasRenderer 累积的绘制元素按纹理分组合并为 UIVertex 合批几何
 * （颜色入顶点，走 host-visible 缓冲，无 staging/waitIdle），
 * 并推入 FUIRenderProxy 交由 RenderViewUI 以 Batched shader 渲染。
 */
class DAPI UUIWidgetComponent : public UPrimitiveComponent {
	DECLARE_CLASS_TYPE(UUIWidgetComponent)

public:
	UUIWidgetComponent(const FString& Name);
	virtual ~UUIWidgetComponent();

	virtual void Tick(float deltaTime) override;
	virtual bool CreateRenderProxy() override;
	virtual void UpdateRenderProxy() override;

public:
	// 设置根 Widget（接管所有权，组件销毁时自动释放整棵树）
	void SetRootWidget(UWidget* InRootWidget);
	UWidget* GetRootWidget() const { return RootWidget; }

	// 释放几何池与 Widget 树（组件销毁时调用）
	void Destroy();

private:
	// 将 CanvasRenderer 累积的元素按纹理分组转为合批几何并上传
	bool RebuildGeometry();

	// 获取纯白色纹理（懒加载），用于纯色矩形
	UTexture* GetWhiteTexture();

	// 逐字段比较两帧 UI 绘制元素是否完全一致（用于静止帧跳过几何重建）
	bool AreElementsEqual(const TArray<UIDrawCallElement>& A, const TArray<UIDrawCallElement>& B) const;

private:
	UWidget* RootWidget = nullptr;
	UCanvasRenderer CanvasRenderer;
	// 合批几何池：每个条目对应一个纹理批次（同纹理元素合并，一次 DrawCall）
	TArray<UGeometry*> GeometryPool;
	TArray<UIBlendGroup> BatchGroups;
	UTexture* WhiteTexture = nullptr;

	// 上次成功上传的绘制元素快照：元素未变化时跳过 RebuildGeometry，
	// 避免每帧重复执行 staging 上传、waitIdle 与材质参数重设（UI 帧率骤降主因）。
	TArray<UIDrawCallElement> CachedElements;
	bool bElementsDirty = true;
};
