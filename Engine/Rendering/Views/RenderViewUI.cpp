#include "RenderViewUI.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Core/Event.hpp"
#include "Math/DMath.hpp"
#include "Framework/Components/TransformComponent.h"
#include "Containers/TArray.hpp"
#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Rendering/Renderer.hpp"
#include "Rendering/Interface/IRenderpass.hpp"
#include "Rendering/Interface/IRendererBackend.hpp"
#include "Framework/Classes/TextActor.h"
#include "Framework/Components/StaticMeshComponent.h"
#include "Rendering/RenderWorld/RenderProxy.h"

static bool RenderViewUIOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	IRenderView* self = (IRenderView*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	switch (code)
	{
	case eEventCode::Default_Rendertarget_Refresh_Required:
		RenderViewSystem::Get().RegenerateRendertargets(self);
		return false;
    default: break;
	}

	return false;
}

RenderViewUI::RenderViewUI(const RenderViewConfig& config) {
	Type = config.type;
	Name = config.name;
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count;
	Passes.Resize(RenderpassCount);
	Renderer = IRenderer::GetRenderer();
}

bool RenderViewUI::OnCreate(const RenderViewConfig& config) {
	// Builtin ui shader.
	const char* ShaderName = "Shader.Builtin.UI";
	UAsset ConfigResource(ShaderName);
	if (!ResourceSystem::Get().Load(ShaderName, EAssetType::Shader, nullptr, &ConfigResource)) {
		GLOG(Log::eError, "Failed to load builtin UI shader.");
		return false;
	}

	FShaderConfig* Config = (FShaderConfig*)ConfigResource.Data;
	// NOTE: Assuming the first pass since that's all this view has.
	if (!ShaderSystem::Get().Create(&Passes[0], Config)) {
		GLOG(Log::eError, "Failed to load builtin UI shader.");
		return false;
	}
	ResourceSystem::Get().Unload(&ConfigResource);

	UsedShader = ShaderSystem::Get().Get(CustomShaderName.IsEmpty() ? ShaderName : CustomShaderName);
	DiffuseMapLocation = UsedShader->GetUniformIndex("diffuse_texture");
	DiffuseColorLocation = UsedShader->GetUniformIndex("diffuse_color");
	ModelLocation = UsedShader->GetUniformIndex("model");

	// 加载 UI 合批着色器（R1 合批路径）。加载失败仅告警，回退原 UI shader，
	// 保证旧逐元素路径可运行（保留原 Shader.Builtin.UI 兼容回退）。
	const char* BatchedShaderName = "Shader.Builtin.UIBatched";
	UAsset BatchedResource(BatchedShaderName);
	if (ResourceSystem::Get().Load(BatchedShaderName, EAssetType::Shader, nullptr, &BatchedResource)) {
		FShaderConfig* BatchedConfig = (FShaderConfig*)BatchedResource.Data;
		if (ShaderSystem::Get().Create(&Passes[0], BatchedConfig)) {
			BatchedShader = ShaderSystem::Get().Get(BatchedShaderName);
			GLOG(Log::eInfo, "RenderViewUI: Batched UI shader '%s' loaded.", BatchedShaderName);
		} else {
			GLOG(Log::eWarn, "RenderViewUI: Failed to create batched UI shader, falling back to legacy UI shader.");
		}
		ResourceSystem::Get().Unload(&BatchedResource);
	} else {
		GLOG(Log::eWarn, "RenderViewUI: Failed to load batched UI shader, falling back to legacy UI shader.");
	}

	// TODO: Set from configurable.
	NearClip = -100.0f;
	FarClip = 100.0f;

	// Default
	ProjectionMatrix = Matrix4::Orthographic(0, config.width, config.height, 0.0f, NearClip, FarClip);
	ViewMatrix = Matrix4::Identity();

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewUIOnEvent)) {
		GLOG(Log::eError, "Unable to listen for refresh required event, creation failed.");
		return false;
	}

	GLOG(Log::eInfo, "Renderview ui created.");
	return true;
}

void RenderViewUI::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewUIOnEvent);
}

void RenderViewUI::OnResize(uint32_t width, uint32_t height) {
	// Check if different. If so, regenerate projection matrix.
	if (width == Width && height == Height) {
		return;
	}

	Width = (uint16_t)width;
	Height = (uint16_t)height;
	ProjectionMatrix = Matrix4::Orthographic(0.0f, (float)Width, (float)Height, 0.0f, NearClip, FarClip);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vector4(0, 0, (float)Width, (float)Height));
	}
}

bool RenderViewUI::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	return true;
}

void RenderViewUI::Render(const TArray<FRenderProxy*>& RenderObejcts) {
	// R4 静态 DrawCall 缓存：计算 UI 代理内容代次（几何重建次数 × 几何数求和），
	// 代次未变且无文本代理时复用缓存数组，跳过每帧 DrawCall 重建与排序。
	// 文本代理无代次机制（每帧可变），存在时强制走重建路径保持正确性。
	uint64_t ContentGen = 0;
	bool bHasTextProxy = false;
	for (FRenderProxy* RenderProxy : RenderObejcts) {
		if (RenderProxy->GetProxyType() == ERenderProxyType::ProxyType_Text) {
			bHasTextProxy = true;
			break;
		}
		if (RenderProxy->GetProxyType() == ERenderProxyType::ProxyType_UI) {
			FUIRenderProxy* UIProxy = static_cast<FUIRenderProxy*>(RenderProxy);
			// 代次 + 几何数共同纳入指纹：新增/移除静止组件或几何数量变化时
			// 即使代次未递增也会使缓存失效，避免命中过期 DrawCall 列表。
			ContentGen += UIProxy->GetContentGeneration() + (uint64_t)UIProxy->GetMesh().Size();
		}
	}

	if (!bUIDrawCacheValid || bHasTextProxy || ContentGen != CachedUIDrawGeneration) {
		TArray<DrawCall> UIDrawCalls;
		// UI draw calls.
		for (FRenderProxy* RenderProxy : RenderObejcts) {
			// 文本代理（原有逻辑）。Cast 宏是 static_cast 无法做动态类型判断，
			// 必须用 ProxyType 显式分支，否则 FUIRenderProxy 会被误判成文本代理。
			if (RenderProxy->GetProxyType() == ERenderProxyType::ProxyType_Text) {
				FTextRenderProxy* TextProxy = static_cast<FTextRenderProxy*>(RenderProxy);
				UGeometry* Geometry = TextProxy->GetMesh();
				if (!Geometry) continue;

				UMaterialInstance* Mat = Geometry->GetMaterialInstance();
				DrawCall dc;
				dc.geometry = Geometry;
				dc.model = TextProxy->GetModelMatrix();
				dc.material = Mat;
				dc.shader = UsedShader;
				dc.userData = nullptr;
				dc.sortKey = ((uint64_t)dc.shader->ID << 32) | (uint64_t)Mat->GetInternalID();

				UIDrawCalls.Push(dc);
				continue;
			}

			// UI 元素代理（Widget 树绘制指令按纹理分组合并为合批几何，
			// 每个批次一次 DrawCall，优先使用 Batched shader）
			if (RenderProxy->GetProxyType() == ERenderProxyType::ProxyType_UI) {
				FUIRenderProxy* UIProxy = static_cast<FUIRenderProxy*>(RenderProxy);
				// 合批 shader 可用时用合批路径；否则回退原 UI shader 逐几何绘制
				UShader* UIShader = BatchedShader ? BatchedShader : UsedShader;
				for (UGeometry* Geometry : UIProxy->GetMesh()) {
					if (!Geometry) continue;

					UMaterialInstance* Mat = Geometry->GetMaterialInstance();
					if (!Mat) {
						continue;
					}

					DrawCall dc;
					dc.geometry = Geometry;
					// UI 顶点已处于屏幕空间，模型矩阵为单位矩阵
					dc.model = Matrix4::Identity();
					dc.material = Mat;
					dc.shader = UIShader;
					dc.userData = nullptr;
					dc.sortKey = ((uint64_t)dc.shader->ID << 32) | (uint64_t)Mat->GetInternalID();

					UIDrawCalls.Push(dc);
				}
				continue;
			}
		}

		UIDrawCalls.Sort([](const DrawCall& a, const DrawCall& b) {
				return a.sortKey < b.sortKey;
			});

		CachedUIDrawCalls = std::move(UIDrawCalls);
		CachedUIDrawGeneration = ContentGen;
		bUIDrawCacheValid = true;
	}

	FFrameData UIData;
	UIData.projection = ProjectionMatrix;
	UIData.view = ViewMatrix;
	UIData.ambieantColor = Vector4(0.0f);
	UIData.cameraPosition = Vector3(0.0f);
	UIData.renderMode = render_mode;
	UIData.time = 0.0f;

	uint8_t RTIndex = Renderer->GetWindowAttachmentIndex();
	uint64_t FrameNumber = Renderer->GetFrameNum();

	Passes[0].Begin(&Passes[0].Targets[RTIndex]);
	Renderer->ExecuteDrawCalls(CachedUIDrawCalls, FrameNumber, UIData);
	Passes[0].End();
}
