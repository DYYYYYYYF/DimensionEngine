#pragma once
#include "Core/CoreMinimal.hpp"
#include "Framework/Object.h"
#include "Containers/TArray.hpp"
#include "Math/MathTypes.hpp"
#include "UCanvasRenderer.h"
#include "UIDrawCallElement.h"

class UWorld;
class URenderWorld;
class UTexture;
class UGeometry;
class FRenderProxy;

/**
 * UI 合批分组：按纹理聚合一批绘制元素，记录批内元素数与上一帧顶点数。
 * 由 UI 宿主（UWidget）在 RebuildGeometry 中填充，用于几何扩容/缩容与索引复用判断。
 */
struct UIBlendGroup {
	UTexture* Texture = nullptr;
	uint32_t ElementCount = 0;
	uint32_t LastVertexCount = 0;
};

/**
 * UI 控件基类：维护 Widget 树（AddChild/ClearChildren），
 * 并定义 UI 核心生命周期：OnMeasure（测量）-> OnLayout（布局）-> OnDraw（绘制）。
 * 坐标系约定：屏幕坐标，原点在窗口左上角，Y 轴向下，单位像素。
 */
class ENGINE_API UWidget : public UObject, public TRequireClassType<UWidget> {
	DECLARE_CLASS_TYPE(UWidget)

public:
	UWidget() : Parent(nullptr), bIsVisible(true) {}
	virtual ~UWidget() { ClearChildren(); }

	// 树状管理
	void AddChild(UWidget* Child) {
		if (Child) {
			Child->Parent = this;
			Children.Push(Child);
		}
	}

	void ClearChildren() {
		for (auto* Child : Children) { delete Child; }
		Children.Clear();
	}

	// 期望尺寸（布局系统据此测量；<=0 表示使用父可用空间/默认值）
	void SetDesiredSize(float InWidth, float InHeight) { DesiredWidth = InWidth; DesiredHeight = InHeight; }
	float GetDesiredWidth() const { return DesiredWidth; }
	float GetDesiredHeight() const { return DesiredHeight; }

	// 屏幕坐标（左上角）
	void SetPosition(float X, float Y) { PositionX = X; PositionY = Y; }
	float GetPositionX() const { return PositionX; }
	float GetPositionY() const { return PositionY; }

	float GetWidth() const { return Width; }
	float GetHeight() const { return Height; }

	void SetVisible(bool bInVisible) { bIsVisible = bInVisible; }
	bool IsVisible() const { return bIsVisible; }

	void SetColor(const Vector4& InColor) { Color = InColor; }
	const Vector4& GetColor() const { return Color; }

	UWidget* GetParent() const { return Parent; }
	const TArray<UWidget*>& GetChildren() const { return Children; }

	// UI核心生命周期：测量 -> 布局 -> 绘制

	// 测量：根据父可用空间确定自身尺寸（默认使用 DesiredSize，未设置时铺满父空间）
	virtual void OnMeasure(float ParentWidth, float ParentHeight) {
		Width = DesiredWidth > 0.0f ? DesiredWidth : ParentWidth;
		Height = DesiredHeight > 0.0f ? DesiredHeight : ParentHeight;
	}

	// 布局：记录自身在屏幕上的矩形（Left/Top 为矩形左上角，Right/Bottom 为右下角）
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) {
		PositionX = Left;
		PositionY = Top;
		Width = Right - Left;
		Height = Bottom - Top;
	}

	// 绘制：将自身绘制指令推入画布，并递归绘制子控件
	virtual void OnDraw(class UCanvasRenderer* InCanvasRenderer) {
		if (!bIsVisible) return;
		// 递归绘制子控件
		for (auto* Child : Children) {
			Child->OnDraw(InCanvasRenderer);
		}
	}

	// ── 输入事件（UE 风格 Widget 交互的最小事件系统） ──────────────

	// 命中测试：判断屏幕坐标是否落在本控件矩形内（容器可覆写裁剪规则）
	virtual bool HitTest(float X, float Y) const {
		if (!bIsVisible) return false;
		return X >= PositionX && X <= PositionX + Width &&
			Y >= PositionY && Y <= PositionY + Height;
	}

	// 点击事件：命中后触发（Button 等交互控件覆写并消费事件）。
	// 返回 true 表示事件已被消费，不再继续冒泡。
	virtual bool OnMouseClick(float X, float Y) { return false; }

	// 从本节点（含子树）分发一次点击：子节点优先（上层覆盖下层），
	// 任一子节点消费则停止。命中自身时触发 OnMouseClick。
	// 返回 true 表示事件已被消费。
	virtual bool DispatchMouseClick(float X, float Y) {
		if (!bIsVisible) return false;
		for (auto* Child : Children) {
			if (!Child) continue;
			if (Child->DispatchMouseClick(X, Y)) return true;
		}
		if (HitTest(X, Y)) {
			return OnMouseClick(X, Y);
		}
		return false;
	}

	// ---- UI 宿主生命周期（取代 UUIWidgetComponent / AUIActor 的宿主职责） ----
	// UWidget 作为 UI 根节点使用时，本节点对象拥有整个 Widget 树（Content 树就是自身 AddChild 结果）。
	// 设置所属 UWorld 并注册代理（FUIRenderProxy 加入 RenderWorld 的屏幕空间 UI 索引）。
	void SetUIWorld(UWorld* InWorld);
	UWorld* GetUIWorld() const { return UIWorld; }

	// 创建并加入 FUIRenderProxy 到 RenderWorld；时机通常由 UWorld::AddUIRoot 流程处理。
	void RegisterUI();
	// 从 RenderWorld 移除并释放几何/代理；时机通常由 UWorld::RemoveUIRoot / UWorld::Destroy 流程处理。
	void UnregisterUI();

	// 每帧 UI 根生命周期：测量 -> 布局 -> 绘制 -> 脏检测（元素/几何/材质并懒更新并上传代理）。
	// 替代 UUIWidgetComponent::Tick 的驱动职责。
	void TickUI(float DeltaTime);

protected:
	UWidget* Parent = nullptr;
	TArray<UWidget*> Children;

	// UI 局部变换数据（屏幕坐标或相对坐标）
	float PositionX = 0.0f, PositionY = 0.0f;
	float Width = 0.0f, Height = 0.0f;

	// 期望尺寸（用于布局测量；<=0 表示自动）
	float DesiredWidth = -1.0f, DesiredHeight = -1.0f;

	// 顶点颜色（默认白色）
	Vector4 Color = Vector4(1.0f);

	bool bIsVisible;

private:
	// 渲染提交状态（迁移自 UUIWidgetComponent）
	UCanvasRenderer CanvasRenderer;
	TArray<UGeometry*> GeometryPool;
	TArray<UIBlendGroup> BatchGroups;
	UTexture* WhiteTexture = nullptr;
	TArray<UIDrawCallElement> CachedElements;
	bool bElementsDirty = true;

	// UI 宿主状态
	UWorld* UIWorld = nullptr;
	FRenderProxy* UIRenderProxy = nullptr;
	bool bUIProxyRegistered = false;

	bool CreateRenderProxy();
	void UpdateRenderProxy();
	UTexture* GetWhiteTexture();
	bool RebuildGeometry();
	bool AreElementsEqual(const TArray<UIDrawCallElement>& A, const TArray<UIDrawCallElement>& B) const;
	void ReleaseUIResources();
};
