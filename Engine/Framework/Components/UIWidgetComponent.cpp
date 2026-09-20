#include "UIWidgetComponent.h"

#include "UI/Widget.h"
#include "UI/UIDrawCallElement.h"
#include "UI/UIVertex.h"
#include "Systems/GeometrySystem.h"
#include "Systems/TextureSystem.h"
#include "Rendering/Renderer.hpp"
#include "Rendering/RenderWorld/RenderProxy.h"
#include "Framework/Classes/Actor.h"

UUIWidgetComponent::UUIWidgetComponent(const FString& Name) : UPrimitiveComponent(Name) {
}

UUIWidgetComponent::~UUIWidgetComponent() {
	Destroy();
}

void UUIWidgetComponent::SetRootWidget(UWidget* InRootWidget) {
	if (RootWidget == InRootWidget) {
		return;
	}

	delete RootWidget;
	RootWidget = InRootWidget;
}

void UUIWidgetComponent::Destroy() {
	// 释放几何池（AcquireFromConfig 以 auto_release 创建，引用归零自动销毁）
	for (UGeometry* Geometry : GeometryPool) {
		if (Geometry) {
			Geometry->DecreaseReferenceCount();
		}
	}
	GeometryPool.Clear();

	// 释放 Widget 树
	delete RootWidget;
	RootWidget = nullptr;

	WhiteTexture = nullptr;
}

bool UUIWidgetComponent::CreateRenderProxy() {
	RenderProxy = NewObject<FUIRenderProxy>(MemoryType::eMemory_Type_Renderer);
	if (!RenderProxy) {
		GLOG(Log::eError, "Failed to create RenderProxy for UIWidgetComponent.");
		return false;
	}

	return true;
}

void UUIWidgetComponent::Tick(float deltaTime) {
	if (!RootWidget) return;

	// 每帧驱动 Widget 树：测量 -> 布局 -> 绘制
	IRenderer* Renderer = IRenderer::GetRenderer();
	const uint32_t ViewportWidth = Renderer->GetWidth();
	const uint32_t ViewportHeight = Renderer->GetHeight();

	CanvasRenderer.Begin(ViewportWidth, ViewportHeight);
	RootWidget->OnMeasure((float)ViewportWidth, (float)ViewportHeight);
	RootWidget->OnLayout(0.0f, 0.0f, (float)ViewportWidth, (float)ViewportHeight);
	RootWidget->OnDraw(&CanvasRenderer);
	CanvasRenderer.End();

	// 脏检测：本帧绘制元素与上次一致时跳过几何/材质重建，直接刷新代理。
	// 静止 UI 帧不再触发每几何的 staging 上传 + 全队列 waitIdle + 材质参数重设，
	// 这是接入 UI 后帧数骤降的关键热路径（旧实现每帧无差别 RebuildGeometry）。
	const TArray<UIDrawCallElement>& Elements = CanvasRenderer.GetElements();
	if (!bElementsDirty && AreElementsEqual(Elements, CachedElements)) {
		UpdateRenderProxy();
		return;
	}

	if (!RebuildGeometry()) {
		// 重建失败：保持脏标记，下一帧重试
		return;
	}

	// 合批几何已重建：递增代理内容代次，使 RenderViewUI 的静态 DrawCall
	// 缓存失效（R4），下一帧 Render 重新构建并排序 DrawCall 数组。
	FUIRenderProxy* UIProxy = Cast<FUIRenderProxy*>(RenderProxy);
	if (UIProxy) {
		UIProxy->SetContentGeneration(UIProxy->GetContentGeneration() + 1);
	}

	CachedElements = Elements;
	bElementsDirty = false;

	UpdateRenderProxy();
}

void UUIWidgetComponent::UpdateRenderProxy() {
	// 还未注册到场景
	if (!IsRegistered) return;

	FUIRenderProxy* Proxy = Cast<FUIRenderProxy*>(RenderProxy);
	if (!Proxy) {
		GLOG(Log::eError, "UUIWidgetComponent RenderProxy is null. Cannot update.");
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) {
		GLOG(Log::eError, "UIWidgetComponent has no owner.");
		return;
	}

	Proxy->SetMesh(GeometryPool);
	Proxy->SetModelMatrix(GetLocalMatrix());
	Proxy->SetUniqueID(Owner->GetUniqueID());
}

UTexture* UUIWidgetComponent::GetWhiteTexture() {
	if (!WhiteTexture) {
		WhiteTexture = TextureSystem::Get().Acquire("white_texture");
	}
	return WhiteTexture;
}

bool UUIWidgetComponent::RebuildGeometry() {
	const TArray<UIDrawCallElement>& Elements = CanvasRenderer.GetElements();
	const uint32_t ElementCount = (uint32_t)Elements.Size();

	// 解析每个元素的有效纹理（nullptr → 白色纹理兜底）
	UTexture* WhiteTex = GetWhiteTexture();
	if (!WhiteTex) {
		WhiteTex = TextureSystem::Get().GetDefaultDiffuseTexture();
	}
	if (!WhiteTex) {
		GLOG(Log::eError, "UUIWidgetComponent: No white/default texture available for UI batching.");
		return false;
	}

	// 1. 按纹理分组（保持首次出现顺序，组间顺序稳定）
	TArray<UTexture*> GroupTextures;
	TArray<uint32_t> GroupCounts;
	for (uint32_t i = 0; i < ElementCount; ++i) {
		UTexture* Tex = Elements[i].Texture ? Elements[i].Texture : WhiteTex;
		int32_t GroupIndex = -1;
		for (uint32_t g = 0; g < GroupTextures.Size(); ++g) {
			if (GroupTextures[g] == Tex) { GroupIndex = (int32_t)g; break; }
		}
		if (GroupIndex < 0) {
			GroupIndex = (int32_t)GroupTextures.Size();
			GroupTextures.Push(Tex);
			GroupCounts.Push(0);
		}
		GroupCounts[GroupIndex]++;
	}

	const uint32_t GroupCount = (uint32_t)GroupTextures.Size();

	// 2. 扩容：为新增批次创建合批几何（UIVertex + Material.UI.Batched + host-visible）
	while (GeometryPool.Size() < GroupCount) {
		const uint32_t Index = (uint32_t)GeometryPool.Size();
		UIVertex TempVertices[4];
		for (uint32_t v = 0; v < 4; ++v) {
			TempVertices[v].position = Vector2(0.0f, 0.0f);
			TempVertices[v].texcoord = Vector2(0.0f, 0.0f);
			TempVertices[v].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		uint32_t TempIndices[6] = { 0, 1, 2, 0, 3, 1 };

		FGeometryConfig Config;
		Config.name = GetOwner() ? GetOwner()->GetName() : FString("UIWidget");
		Config.vertex_size = sizeof(UIVertex);
		Config.vertex_count = 4;
		Config.vertices = TempVertices;
		Config.index_size = sizeof(uint32_t);
		Config.index_count = 6;
		Config.indices = TempIndices;
		Config.material_name = "Material.UI.Batched";
		Config.bHostVisible = true;

		UGeometry* Geometry = GeometrySystem::Get().AcquireFromConfig(Config, true);
		if (!Geometry) {
			GLOG(Log::eError, "UUIWidgetComponent: Failed to acquire UI batch geometry.");
			return false;
		}

		GeometryPool.Push(Geometry);
		BatchGroups.Push(UIBlendGroup());
		(void)Index;
	}

	// 3. 缩容：释放多余批次几何
	while (GeometryPool.Size() > GroupCount) {
		const uint32_t Last = (uint32_t)GeometryPool.Size() - 1;
		if (GeometryPool[Last]) {
			GeometryPool[Last]->DecreaseReferenceCount();
		}
		GeometryPool.PopAt(Last);
		BatchGroups.PopAt(Last);
	}

	IRenderer* Renderer = IRenderer::GetRenderer();

	// 4. 逐批次填充顶点并上传
	for (uint32_t g = 0; g < GroupCount; ++g) {
		const UTexture* GroupTex = GroupTextures[g];
		const uint32_t GroupElementCount = GroupCounts[g];
		const uint32_t GroupVertexCount = GroupElementCount * 4;
		const uint32_t GroupIndexCount = GroupElementCount * 6;

		// 收集该批次全部元素顶点（屏幕空间四边形，Y 向下，V=0 在顶部）
		TArray<UIVertex> Vertices;
		Vertices.Resize(GroupVertexCount);
		UIVertex* VertexData = Vertices.Data();

		uint32_t ElementInGroup = 0;
		for (uint32_t i = 0; i < ElementCount; ++i) {
			UTexture* Tex = Elements[i].Texture ? Elements[i].Texture : WhiteTex;
			if (Tex != GroupTex) continue;

			const UIDrawCallElement& Element = Elements[i];
			const float MinX = Element.PositionX;
			const float MinY = Element.PositionY;
			const float MaxX = Element.PositionX + Element.Width;
			const float MaxY = Element.PositionY + Element.Height;

			const uint32_t Base = ElementInGroup * 4;
			VertexData[Base + 0] = UIVertex(Vector2(MinX, MinY), Element.UV0, Element.Color);
			VertexData[Base + 1] = UIVertex(Vector2(MaxX, MaxY), Element.UV1, Element.Color);
			VertexData[Base + 2] = UIVertex(Vector2(MaxX, MinY), Vector2(Element.UV1.x, Element.UV0.y), Element.Color);
			VertexData[Base + 3] = UIVertex(Vector2(MinX, MaxY), Vector2(Element.UV0.x, Element.UV1.y), Element.Color);

			ElementInGroup++;
		}

		// 元素数变化时重新上传索引（首建/组增长/组缩小）；不变时省略索引复用旧块
		const bool bIndexChanged = (BatchGroups[g].LastVertexCount != GroupVertexCount);

		TArray<uint32_t> Indices;
		uint32_t* IndexData = nullptr;
		if (bIndexChanged) {
			Indices.Resize(GroupIndexCount);
			IndexData = Indices.Data();
			for (uint32_t e = 0; e < GroupElementCount; ++e) {
				const uint32_t Base = e * 6;
				const uint32_t VBase = e * 4;
				IndexData[Base + 0] = VBase + 0;
				IndexData[Base + 1] = VBase + 1;
				IndexData[Base + 2] = VBase + 2;
				IndexData[Base + 3] = VBase + 0;
				IndexData[Base + 4] = VBase + 3;
				IndexData[Base + 5] = VBase + 1;
			}
		}

		FGeometryConfig Config;
		Config.vertex_size = sizeof(UIVertex);
		Config.vertex_count = GroupVertexCount;
		Config.vertices = VertexData;
		Config.index_size = sizeof(uint32_t);
		Config.index_count = bIndexChanged ? GroupIndexCount : 0;
		Config.indices = IndexData;
		Config.bHostVisible = true;

		UGeometry* Geometry = GeometryPool[g];
		if (!Renderer->CreateGeometry(Geometry, Config)) {
			GLOG(Log::eError, "UUIWidgetComponent: Failed to update UI batch geometry.");
			return false;
		}

		// 批次材质参数：diffuse_color 恒白（颜色在顶点），diffuse_texture = 组纹理
		UMaterialInstance* MaterialInstance = Geometry->GetMaterialInstance();
		if (MaterialInstance) {
			Vector4 White(1.0f, 1.0f, 1.0f, 1.0f);
			MaterialInstance->SetUniformValue("diffuse_color", &White, sizeof(Vector4));

			FTextureMap TextureMap;
			TextureMap.texture = (UTexture*)GroupTex;
			MaterialInstance->SetTextureOnBinding("diffuse_texture", TextureMap);
		}

		BatchGroups[g].Texture = GroupTextures[g];
		BatchGroups[g].ElementCount = GroupElementCount;
		BatchGroups[g].LastVertexCount = GroupVertexCount;
	}

	return true;
}

bool UUIWidgetComponent::AreElementsEqual(const TArray<UIDrawCallElement>& A, const TArray<UIDrawCallElement>& B) const {
	if (A.Size() != B.Size()) {
		return false;
	}

	for (size_t i = 0; i < A.Size(); ++i) {
		const UIDrawCallElement& a = A[i];
		const UIDrawCallElement& b = B[i];
		// 逐字段精确比较：同一 Widget 树在同一输入下按相同计算路径得到确定浮点值，
		// 静止帧元素完全相等；任一字段变化（位置/尺寸/UV/颜色/纹理）均触发重建。
		if (a.PositionX != b.PositionX || a.PositionY != b.PositionY ||
			a.Width != b.Width || a.Height != b.Height ||
			a.UV0.x != b.UV0.x || a.UV0.y != b.UV0.y ||
			a.UV1.x != b.UV1.x || a.UV1.y != b.UV1.y ||
			a.Color.x != b.Color.x || a.Color.y != b.Color.y ||
			a.Color.z != b.Color.z || a.Color.w != b.Color.w ||
			a.Texture != b.Texture) {
			return false;
		}
	}

	return true;
}
