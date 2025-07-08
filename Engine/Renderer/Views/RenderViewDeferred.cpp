#include "RenderViewDeferred.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/Event.hpp"
#include "Core/DMemory.hpp"
#include "Core/UID.hpp"
#include "Math/DMath.hpp"
#include "Math/Transform.hpp"
#include "Containers/TArray.hpp"
#include "Containers/TString.hpp"
#include "Containers/FString.hpp"

#include "Systems/MaterialSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Systems/TextureSystem.h"
#include "Systems/GeometrySystem.h"

#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
#include "Renderer/Interface/IRendererBackend.hpp"

static bool RenderViewWorldDeferredOnEvent(eEventCode code, void* sender, void* listenerInst, SEventContext context) {
	IRenderView* self = (IRenderView*)listenerInst;
	if (self == nullptr) {
		return false;
	}

	switch ((eEventCode)code) {
	case eEventCode::Default_Rendertarget_Refresh_Required: {
		RenderViewSystem::RegenerateRendertargets(self);
		return true;
	}

	case eEventCode::Set_Render_Mode: {
		int RenderMode = context.data.i32[0];
		switch (RenderMode) {
		case ShaderRenderMode::eShader_Render_Mode_Default:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Default;
			GLOG(Log::eDebug, "Change render mode: eShader_Render_Mode_Default.");
			break;
		case ShaderRenderMode::eShader_Render_Mode_Lighting:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Lighting;
			GLOG(Log::eDebug, "Change render mode: eShader_Render_Mode_Lighting.");
			break;
		case ShaderRenderMode::eShader_Render_Mode_Normals:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Normals;
			GLOG(Log::eDebug, "Change render mode: eShader_Render_Mode_Normals.");
			break;
		case ShaderRenderMode::eShader_Render_Mode_Depth:
			self->render_mode = ShaderRenderMode::eShader_Render_Mode_Depth;
			GLOG(Log::eDebug, "Change render mode: eShader_Render_Mode_Depth.");
			break;
		}
		return true;
	}
	default: return true;
	}

	return false;
}

RenderViewWorldDeferred::RenderViewWorldDeferred() {
	FullscreenQuad = nullptr;
	Renderer = nullptr;
}

RenderViewWorldDeferred::RenderViewWorldDeferred(const RenderViewConfig& config) {
	Type = config.type;
	Name = StringCopy(config.name);
	CustomShaderName = config.custom_shader_name;
	RenderpassCount = config.pass_count; // 应该是2个通道：G-Buffer和光照
	Passes.resize(RenderpassCount);
	FullscreenQuad = nullptr;
	Renderer = IRenderer::GetRenderer();
}

bool RenderViewWorldDeferred::OnCreate(const RenderViewConfig& config) {
	// 加载G-Buffer着色器
	const char* GBufferShaderName = "Shader.Builtin.GBuffer";
	Resource ConfigResource;
	if (!ResourceSystem::Load(GBufferShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		GLOG(Log::eError, "Failed to load builtin G-Buffer shader.");
		return false;
	}

	ShaderConfig* Config = (ShaderConfig*)ConfigResource.Data;
	// 第一个通道用于G-Buffer渲染
	if (!ShaderSystem::Create(&Passes[0], Config)) {
		GLOG(Log::eError, "Failed to create G-Buffer shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);

	GBufferShader = ShaderSystem::Get(GBufferShaderName);

	// 加载延迟光照着色器
	const char* LightingShaderName = "Shader.Builtin.DeferredLighting";
	if (!ResourceSystem::Load(LightingShaderName, ResourceType::eResource_Type_Shader, nullptr, &ConfigResource)) {
		GLOG(Log::eError, "Failed to load builtin deferred lighting shader.");
		return false;
	}

	Config = (ShaderConfig*)ConfigResource.Data;
	// 第二个通道用于光照计算
	if (!ShaderSystem::Create(&Passes[1], Config)) {
		GLOG(Log::eError, "Failed to create deferred lighting shader.");
		return false;
	}
	ResourceSystem::Unload(&ConfigResource);

	LightingShader = ShaderSystem::Get(LightingShaderName);

	// 设置渲染参数 (与原World渲染保持一致)
	NearClip = 0.1f;
	FarClip = 1000.0f;
	Fov = Deg2Rad(45.0f);

	ProjectionMatrix = Matrix4::Perspective(Fov, (float)config.width / config.height, NearClip, FarClip);
	WorldCamera = CameraSystem::GetDefault();

	// 环境光设置 (与原World渲染保持一致)
	AmbientColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);

	// 创建G-Buffer纹理
	if (!CreateGBufferTextures(config.width, config.height)) {
		GLOG(Log::eError, "Failed to create G-Buffer textures.");
		return false;
	}

	// 创建延迟光照材质
	/*if (!MaterialSystem::CreateDeferredLightingMaterial(&AlbedoTexture, &NormalTexture, &PositionTexture)) {
		GLOG(Log::eError, "Failed to create deferred lighting material.");
		return false;
	}*/

	if (!EngineEvent::Register(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewWorldDeferredOnEvent)) {
		GLOG(Log::eError, "Unable to listen for refresh required event, creation failed.");
		return false;
	}
	if (!EngineEvent::Register(eEventCode::Set_Render_Mode, this, RenderViewWorldDeferredOnEvent)) {
		GLOG(Log::eError, "Unable to listen for render mode event, creation failed.");
		return false;
	}

	GLOG(Log::eInfo, "Deferred world render view created successfully.");
	return true;
}

void RenderViewWorldDeferred::OnDestroy() {
	EngineEvent::Unregister(eEventCode::Default_Rendertarget_Refresh_Required, this, RenderViewWorldDeferredOnEvent);
	EngineEvent::Unregister(eEventCode::Set_Render_Mode, this, RenderViewWorldDeferredOnEvent);

	DestroyGBufferTextures();
	DestroyFullscreenQuad();
}

void RenderViewWorldDeferred::OnResize(uint32_t width, uint32_t height) {
	if (width == Width && height == Height) {
		return;
	}

	Width = width;
	Height = height;
	ProjectionMatrix = Matrix4::Perspective(Fov, (float)Width / (float)Height, NearClip, FarClip);

	// 重新创建G-Buffer纹理
	DestroyGBufferTextures();
	CreateGBufferTextures(width, height);

	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vector4(0, 0, (float)Width, (float)Height));
	}

	// 更新渲染通道的渲染区域
	for (uint32_t i = 0; i < RenderpassCount; ++i) {
		Passes[i].SetRenderArea(Vector4(0, 0, (float)Width, (float)Height));

		// 更新所有渲染目标的纹理引用
		for (uint32_t targetIndex = 0; targetIndex < Passes[i].Targets.size(); ++targetIndex) {
			RenderTarget* target = &Passes[i].Targets[targetIndex];

			if (i == 0) {
				// 更新G-Buffer纹理引用
				uint32_t bufferIndex = targetIndex % MAX_RENDER_TARGETS;
				for (uint32_t attachIndex = 0; attachIndex < target->attachments.size(); ++attachIndex) {
					RenderTargetAttachment* attachment = &target->attachments[attachIndex];

					if (attachment->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color) {
						switch (attachment->index) {
						case 0:
							attachment->texture = GBuffers[bufferIndex].AlbedoTexture;
							break;
						case 1:
							attachment->texture = GBuffers[bufferIndex].NormalTexture;
							break;
						case 2:
							attachment->texture = GBuffers[bufferIndex].PositionTexture;
							break;
						}
					}
					else if (attachment->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth) {
						attachment->texture = GBuffers[bufferIndex].DepthTexture;
					}
				}
			}
			// 光照通道使用swapchain，无需更新纹理引用
		}
	}
}

bool RenderViewWorldDeferred::OnBuildPacket(IRenderviewPacketData* data, struct RenderViewPacket* out_packet) {
	if (data == nullptr || out_packet == nullptr) {
		GLOG(Log::eWarn, "RenderViewWorldDeferred::OnBuildPacket() Requires valid pointer to packet and data.");
		return false;
	}

	WorldPacketData* Data = (WorldPacketData*)data;
	const std::vector<GeometryRenderData> GeometryData = Data->Meshes;
	out_packet->view = this;

	// 设置矩阵等
	out_packet->projection_matrix = ProjectionMatrix;
	out_packet->view_matrix = WorldCamera->GetViewMatrix();
	out_packet->view_position = WorldCamera->GetPosition();
	out_packet->ambient_color = AmbientColor;
	out_packet->global_time = Data->GlobalTime;

	// 获取所有几何体
	uint32_t GeometryDataCount = (uint32_t)GeometryData.size();
	for (uint32_t i = 0; i < GeometryDataCount; ++i) {
		const GeometryRenderData& GData = GeometryData[i];
		if (GData.geometry == nullptr) {
			continue;
		}

		// 延迟渲染中，不需要按透明度排序，统一处理
		out_packet->geometries.push_back(GeometryData[i]);
		out_packet->geometry_count++;
	}

	return true;
}

void RenderViewWorldDeferred::OnDestroyPacket(struct RenderViewPacket* packet) {
	packet->geometries.clear();
	std::vector<GeometryRenderData>().swap(packet->geometries);
}

bool RenderViewWorldDeferred::RegenerateAttachmentTarget(uint32_t passIndex, RenderTargetAttachment* attachment) {
	if (passIndex == 0) {
		// G-Buffer通道 - 多个渲染目标
		if (attachment->type & eRender_Target_Attachment_Type_Color) {
			// 根据attachment的索引来决定使用哪个G-Buffer纹理
			// 注意：这里需要为每个render_target_index创建对应的attachment
			uint32_t bufferIndex = 0; // 默认使用第一个缓冲，实际应该根据当前渲染目标索引确定

			switch (attachment->index) {
			case 0:
				attachment->texture = GBuffers[bufferIndex].AlbedoTexture;
				GLOG(Log::eDebug, "G-Buffer: Binding Albedo texture to color attachment %d", attachment->index);
				break;
			case 1:
				attachment->texture = GBuffers[bufferIndex].NormalTexture;
				GLOG(Log::eDebug, "G-Buffer: Binding Normal texture to color attachment %d", attachment->index);
				break;
			case 2:
				attachment->texture = GBuffers[bufferIndex].PositionTexture;
				GLOG(Log::eDebug, "G-Buffer: Binding Position texture to color attachment %d", attachment->index);
				break;
			default:
				GLOG(Log::eError, "G-Buffer: Unsupported color attachment index %d", attachment->index);
				return false;
			}
		}
		else if (attachment->type & eRender_Target_Attachment_Type_Depth) {
			uint32_t bufferIndex = 0; // 默认使用第一个缓冲
			attachment->texture = GBuffers[bufferIndex].DepthTexture;
			GLOG(Log::eDebug, "G-Buffer: Binding Depth texture");
		}
		else {
			GLOG(Log::eError, "G-Buffer: Unsupported attachment type %d", attachment->type);
			return false;
		}
	}
	else if (passIndex == 1) {
		// 光照通道 - 使用默认颜色缓冲或者swapchain
		// 这里通常不需要重新生成attachment，直接使用默认的
		if ((attachment->type & eRender_Target_Attachment_Type_Color) && attachment->index == 0) {
			// 主颜色输出，通常是swapchain图像
			GLOG(Log::eDebug, "Lighting Pass: Using default color attachment");
			return true;
		}
	}
	else {
		GLOG(Log::eError, "Unsupported pass index %d for deferred rendering", passIndex);
		return false;
	}

	return true;
}

bool RenderViewWorldDeferred::OnRender(struct RenderViewPacket* packet, IRendererBackend* back_renderer, size_t frame_number, size_t render_target_index) {
	// 创建全屏四边形
	if (!FullscreenQuad) {
		if (!CreateFullscreenQuad()) {
			GLOG(Log::eError, "Failed to create fullscreen quad.");
			return false;
		}
	}

	// 获取当前GBuffer纹理组
	GBufferSet* CurrentGBuffer = GetCurrentGBufferSet(render_target_index);

	// 第一通道：G-Buffer渲染
	IRenderpass* GBufferPass = (IRenderpass*)&Passes[0];
	GBufferPass->Begin(&GBufferPass->Targets[render_target_index]);

	if (!ShaderSystem::UseByID(GBufferShader->ID)) {
		GLOG(Log::eError, "Failed to use G-Buffer shader.");
		GBufferPass->End();
		return false;
	}

	// 应用全局uniform 
	ShaderSystem::SetUniform("projection", &packet->projection_matrix);
	ShaderSystem::SetUniform("view", &packet->view_matrix);
	ShaderSystem::SetUniform("view_position", &packet->view_position);
	ShaderSystem::SetUniform("mode", &render_mode);
	ShaderSystem::SetUniform("time", &packet->global_time);
	ShaderSystem::ApplyGlobal();

	// 渲染所有几何体到G-Buffer
	uint32_t Count = packet->geometry_count;
	for (uint32_t i = 0; i < Count; ++i) {
		Material* Mat = packet->geometries[i].geometry->Material
			? packet->geometries[i].geometry->Material
			: MaterialSystem::GetDefaultMaterial();

		// 应用材质
		bool IsNeedUpdate = Mat->RenderFrameNumer != frame_number;
		if (!MaterialSystem::ApplyInstance(Mat, IsNeedUpdate)) {
			GLOG(Log::eWarn, "Failed to apply G-Buffer material '%s'. Skipping draw.", Mat->Name.c_str());
			continue;
		}
		Mat->RenderFrameNumer = (uint32_t)frame_number;

		// 应用模型矩阵
		MaterialSystem::ApplyLocal(Mat, packet->geometries[i].model);

		// 绘制几何体
		back_renderer->DrawGeometry(&packet->geometries[i]);
	}
	GBufferPass->End();

	// 第二通道：延迟光照
	IRenderpass* LightingPass = (IRenderpass*)&Passes[1];
	LightingPass->Begin(&LightingPass->Targets[render_target_index]);

	if (!ShaderSystem::UseByID(LightingShader->ID)) {
		GLOG(Log::eError, "Failed to use deferred lighting shader.");
		LightingPass->End();
		return false;
	}

	// 应用全局uniform (环境光、视角位置等) - 延迟光照专用接口
	ShaderSystem::SetUniform("projection", &packet->projection_matrix);
	ShaderSystem::SetUniform("view", &packet->view_matrix);
	ShaderSystem::SetUniform("view_position", &packet->view_position);
	ShaderSystem::SetUniform("mode", &render_mode);
	ShaderSystem::SetUniform("time", &packet->global_time);
	ShaderSystem::ApplyGlobal();

	// 创建延迟光照材质并应用
	Material* DeferredLightingMat = FullscreenQuad->Material;
	// 绑定G-Buffer纹理作为材质纹理
	DeferredLightingMat->DiffuseMap.texture = CurrentGBuffer->AlbedoTexture;
	DeferredLightingMat->NormalMap.texture = CurrentGBuffer->NormalTexture;
	DeferredLightingMat->SpecularMap.texture = CurrentGBuffer->PositionTexture;

	// 应用延迟光照材质
	bool LightingNeedsUpdate = DeferredLightingMat->RenderFrameNumer != frame_number;
	if (LightingNeedsUpdate) {
		ShaderSystem::BindInstance(DeferredLightingMat->InternalID);
		ShaderSystem::SetUniform("albedo_texture", &CurrentGBuffer->AlbedoTextureMap);		 // 复用DiffuseMap绑定点
		ShaderSystem::SetUniform("normal_texture", &CurrentGBuffer->NormalTextureMap);		 // 复用NormalMap绑定点  
		ShaderSystem::SetUniform("position_texture", &CurrentGBuffer->PositionTextureMap);	 // 复用SpecularMap绑定点
		ShaderSystem::ApplyInstance(LightingNeedsUpdate);
	}
	else {
		DeferredLightingMat->RenderFrameNumer = (uint32_t)frame_number;
	}

	// 渲染全屏四边形进行光照计算
	GeometryRenderData QuadRenderData;
	QuadRenderData.geometry = FullscreenQuad;
	QuadRenderData.model = Matrix4::Identity();
	back_renderer->DrawGeometry(&QuadRenderData);

	LightingPass->End();

	return true;
}

bool RenderViewWorldDeferred::CreateGBufferTextures(uint32_t width, uint32_t height) {
	for (uint32_t bufferIndex = 0; bufferIndex < MAX_RENDER_TARGETS; ++bufferIndex) {
		// 创建反照率+金属度纹理 (RGBA8)
		FString ALbedoTextureName("GBuffer_Albedo_%d", bufferIndex);
		GBuffers[bufferIndex].AlbedoTexture = TextureSystem::AcquireWriteable(ALbedoTextureName.CStr(), width, height, 4, false);
		GBuffers[bufferIndex].AlbedoTextureMap.texture = GBuffers[bufferIndex].AlbedoTexture;
		Renderer->AcquireTextureMap(&GBuffers[bufferIndex].AlbedoTextureMap);

		// 创建法线+粗糙度纹理 (RGBA16F)
		FString NormalTextureName("GBuffer_Normal_%d", bufferIndex);
		GBuffers[bufferIndex].NormalTexture = TextureSystem::AcquireWriteable(NormalTextureName.CStr(), width, height, 4, false);
		GBuffers[bufferIndex].NormalTextureMap.texture = GBuffers[bufferIndex].NormalTexture;
		Renderer->AcquireTextureMap(&GBuffers[bufferIndex].NormalTextureMap);

		// 创建位置纹理 (RGBA32F)
		FString PositionTextureName("GBuffer_Position_%d", bufferIndex);
		GBuffers[bufferIndex].PositionTexture = TextureSystem::AcquireWriteable(PositionTextureName.CStr(), width, height, 4, false);
		GBuffers[bufferIndex].PositionTextureMap.texture = GBuffers[bufferIndex].PositionTexture;
		Renderer->AcquireTextureMap(&GBuffers[bufferIndex].PositionTextureMap);

		// 创建深度纹理
		FString DepthTextureName("GBuffer_Depth_%d", bufferIndex);
		GBuffers[bufferIndex].DepthTexture = TextureSystem::AcquireWriteable(DepthTextureName.CStr(), width, height, 1, false);
		GBuffers[bufferIndex].DepthTextureMap.texture = GBuffers[bufferIndex].DepthTexture;
		Renderer->AcquireTextureMap(&GBuffers[bufferIndex].DepthTextureMap);
	}

	return true;
}

void RenderViewWorldDeferred::DestroyGBufferTextures() {
	// 销毁G-Buffer纹理
	for (uint32_t bufferIndex = 0; bufferIndex < MAX_RENDER_TARGETS; ++bufferIndex) {
		TextureSystem::Release(GBuffers[bufferIndex].AlbedoTexture->GetName());
		TextureSystem::Release(GBuffers[bufferIndex].NormalTexture->GetName());
		TextureSystem::Release(GBuffers[bufferIndex].PositionTexture->GetName());
		TextureSystem::Release(GBuffers[bufferIndex].DepthTexture->GetName());
	}
}

bool RenderViewWorldDeferred::CreateFullscreenQuad() {
	// 创建全屏四边形几何体
	FullscreenQuad = GeometrySystem::GenerateQuad("DFFullScreenQuad", "Material.Builtin.DeferredLighting");
	return FullscreenQuad != nullptr;
}

void RenderViewWorldDeferred::DestroyFullscreenQuad() {
	if (FullscreenQuad) {
		GeometrySystem::Release(FullscreenQuad);
	}
}