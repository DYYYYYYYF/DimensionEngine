#include "RenderViewSystem.hpp"

#include "Containers/THashTable.hpp"
#include "Containers/TString.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"

// TODO: temp
#include "Renderer/Views/RenderViewUI.hpp"
#include "Renderer/Views/RenderViewWorld.hpp"
#include "Renderer/Views/RenderViewSkybox.hpp"

HashTable RenderViewSystem::Lookup;
void* RenderViewSystem::TableBlock = nullptr;
uint32_t RenderViewSystem::MaxViewCount = 0;
TArray<IRenderView*> RenderViewSystem::RegisteredViews;
bool RenderViewSystem::Initialized = false;
IRenderer* RenderViewSystem::Renderer = nullptr;

bool RenderViewSystem::Initialize(IRenderer* renderer, SRenderViewSystemConfig config) {
	if (renderer == nullptr) {
		LOG_FATAL("RenderViewSystem::Initialize() Invalid renderer pointer.");
		return false;
	}

	if (config.max_view_count == 0) {
		LOG_FATAL("RenderViewSystem::Initialize() Config.max_view_count must be > 0.");
		return false;
	}

	MaxViewCount = config.max_view_count;
	Renderer = renderer;

	TableBlock = Memory::Allocate(sizeof(unsigned short) * MaxViewCount, MemoryType::eMemory_Type_Array);
	Lookup.Create(sizeof(unsigned short), MaxViewCount, TableBlock, false);
	unsigned short InvalidID = INVALID_ID_U16;
	Lookup.Fill(&InvalidID);

	// Fill the array with invalid entries.
	RegisteredViews.Resize(MaxViewCount);
	for (uint32_t i = 0; i < MaxViewCount; ++i) {
		RegisteredViews[i] = nullptr;
	}

	Initialized = true;
	return true;
}

void RenderViewSystem::Shutdown() {
	RegisteredViews.Clear();
}

bool RenderViewSystem::Create(const RenderViewConfig& config) {
	if (config.pass_count < 1) {
		LOG_ERROR("RenderViewSystem::Create() Renderpass count is zero.");
		return false;
	}

	if (!config.name || strlen(config.name) < 1) {
		LOG_ERROR("RenderViewSystem::Create() name is required.");
		return false;
	}

	unsigned short ID = INVALID_ID_U16;
	Lookup.Get(config.name, &ID);
	if (ID != INVALID_ID_U16) {
		LOG_ERROR("RenderViewSystem::Create() A view named '%s' already exists. A new one will not be created.", config.name);
		return false;
	}

	// Find a new ID.
	for (uint32_t i = 0; i < MaxViewCount; ++i) {
		if (RegisteredViews[i] == nullptr) {
			ID = i;
			break;
		}
	}

	// Make sure id was valid.
	if (ID == INVALID_ID_U16) {
		LOG_ERROR("RenderViewSystem::Create() No available space for a new view. Change system config to account for more.");
		return false;
	}

	// TODO: Assign these function pointers to known functions based on the view type.
	// TODO: Refactor pattern.
	if (config.type == RenderViewKnownType::eRender_View_Known_Type_World) {
		RegisteredViews[ID] = new RenderViewWorld(config);
	}
	else if (config.type == RenderViewKnownType::eRender_View_Known_Type_UI) {
		RegisteredViews[ID] = new RenderViewUI(config);
	}
	else if (config.type == RenderViewKnownType::eRender_View_Known_Type_Skybox) {
		RegisteredViews[ID] = new RenderViewSkybox(config);
	}

	IRenderView* View = RegisteredViews[ID];
	View->ID = ID;

	for (uint32_t i = 0; i < View->RenderpassCount; ++i) {
		if (!Renderer->CreateRenderpass(&View->Passes[i], &config.passes[i])) {
			LOG_FATAL("RenderViewSystem::Create() Renderpass not found: '%s'.", config.passes[i].name);
			return false;
		}
	}

	View->OnCreate(config);
	RegenerateRendertargets(View);

	// Update the hashtable entry.
	Lookup.Set(config.name, &ID);

	return true;
}

void RenderViewSystem::RegenerateRendertargets(IRenderView* view) {
	// Create render target for each.
	for (size_t r = 0; r < view->RenderpassCount; ++r) {
		IRenderpass* Pass = &view->Passes[r];

		for (unsigned char i = 0; i < Pass->RenderTargetCount; ++i) {
			RenderTarget* Target = &Pass->Targets[i];
			// Destroy the old if exists.
			// TODO: check if a resize is actually needed for this target.
			Renderer->DestroyRenderTarget(Target, false);

			for (uint32_t a = 0; a < Target->attachment_count; ++a) {
				RenderTargetAttachment* Attachment = &Target->attachments[a];
				if (Attachment->source == RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default) {
					if (Attachment->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color) {
						Attachment->texture = Renderer->GetWindowAttachment(i);
					}
					else if (Attachment->type == RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth) {
						Attachment->texture = Renderer->GetDepthAttachment(i);
					}
					else {
						LOG_FATAL("Unsupported attachment type: 0x%x", Attachment->type);
						continue;
					}
				}
				else if (Attachment->source == RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View) {
					if (!view->RegenerateAttachmentTarget((uint32_t)r, Attachment)) {
						LOG_ERROR("View failed to regenerate attachment target for attachment type: 0x%x.", Attachment->type);
					}
				}
			}

			// Create the render target.
			Renderer->CreateRenderTarget(Target->attachment_count, Target->attachments,
				Pass, Target->attachments[0].texture->Width, Target->attachments[0].texture->Height,
				&Pass->Targets[i]);
		}
	}
}

void RenderViewSystem::OnWindowResize(uint32_t width, uint32_t height) {
	// Send to all view.
	for (uint32_t i = 0; i < RegisteredViews.Size(); ++i) {
		if (RegisteredViews[i]) {
			RegisteredViews[i]->OnResize(width, height);
		}
	}
}

IRenderView* RenderViewSystem::Get(const char* name) {
	if (Initialized) {
		unsigned short ID = INVALID_ID_U16;
		Lookup.Get(name, &ID);
		if (ID != INVALID_ID_U16) {
			IRenderView* Result = RegisteredViews[ID];
			return Result;
		}
	}

	return nullptr;
}

bool RenderViewSystem::BuildPacket(const IRenderView* view, void* data, struct RenderViewPacket* out_packet) {
	if (out_packet && view) {
		return view->OnBuildPacket(data, out_packet);
	}

	return false;
}

bool RenderViewSystem::OnRender(const IRenderView* view, RenderViewPacket* packet, size_t frame_number, size_t render_target_index) {
	if (view && Renderer) {
		return view->OnRender(packet, Renderer->GetRenderBackend(), frame_number, render_target_index);
	}

	return false;
}
