#include "Game.hpp"

#include <Core/EngineLogger.hpp>
#include <Core/Input.hpp>
#include <Core/Event.hpp>
#include <Core/Metrics.hpp>
#include <Systems/CameraSystem.h>
#include <Containers/TString.hpp>

// TODO: Temp
#include <Systems/GeometrySystem.h>
#include <Systems/TextureSystem.h>
#include <Systems/ShaderSystem.h>
#include <Systems/RenderViewSystem.hpp>
#include <Core/Identifier.hpp>
#include <Renderer/RendererFrontend.hpp>

bool ConfigureRenderviews(SApplicationConfig* config);

bool GameOnEvent(unsigned short code, void* sender, void* listender_inst, SEventContext context) {
	SGame* GameInstance = (SGame*)listender_inst;
	SGameState* State = (SGameState*)GameInstance->state;

	switch (code)
	{
	case Core::eEvent_Code_Object_Hover_ID_Changed: {
		State->HoveredObjectID = context.data.u32[0];
		return true;
	}
	}

	return false;
}

bool GameOnDebugEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	SGame* GameInstance = (SGame*)listener_instance;
	SGameState* State = (SGameState*)GameInstance->state;

	if (code == Core::eEvent_Code_Debug_0) {
		if (!State->ModelsLoaded) {
			LOG_DEBUG("Loading models...");

			if (!State->CarMesh->LoadFromResource("falcon")) {
				LOG_ERROR("Failed to load falcon mesh!");
			}

			if (!State->SponzaMesh->LoadFromResource("sponza")) {
				LOG_ERROR("Failed to load sponza mesh!");
			}

			State->ModelsLoaded = true;
		}

		return true;
	}

	return false;
}


bool GameOnKey(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	if (code == Core::eEvent_Code_Key_Pressed) {
		unsigned short KeyCode = context.data.u16[0];
		if (KeyCode == eKeys_Escape) {
			// NOTE: Technically dispatch an event to itself, but there may be other listeners.
			SEventContext data = {};
			Core::EventFire(Core::eEvent_Code_Application_Quit, 0, data);

			// Block anything else from processing this.
			return true;
		}
	}
	else if (code == Core::eEvent_Code_Key_Released) {

		return true;
	}

	return false;
}

bool GameBoot(SGame* gameInstance, IRenderer* renderer) {
	LOG_INFO("Booting...");

	Renderer = renderer;
	SApplicationConfig* Config = &gameInstance->app_config;
	
	// Configure fonts.
	BitmapFontConfig BmpFontConfig;
	BmpFontConfig.name = "Ubuntu Mono 21px";
	BmpFontConfig.resourceName = "UbuntuMono21px";
	BmpFontConfig.size = 21;

	SystemFontConfig SysFontConfig;
	SysFontConfig.defaultSize = 20;
	SysFontConfig.name = "Noto Sans";
	SysFontConfig.resourceName = "NotoSansCJK";

	Config->FontConfig.autoRelease = false;
	Config->FontConfig.defaultBitmapFontCount = 1;
	Config->FontConfig.bitmapFontConfigs = (BitmapFontConfig*)Memory::Allocate(sizeof(BitmapFontConfig) * 1, MemoryType::eMemory_Type_Array);
	Config->FontConfig.bitmapFontConfigs[0] = BmpFontConfig;
	Config->FontConfig.defaultSystemFontCount = 1;
	Config->FontConfig.systemFontConfigs = (SystemFontConfig*)Memory::Allocate(sizeof(SystemFontConfig) * 1, MemoryType::eMemory_Type_Array);
	Config->FontConfig.systemFontConfigs[0] = SysFontConfig;
	Config->FontConfig.maxBitmapFontCount = 100;
	Config->FontConfig.maxSystemFontCount = 100;

	// Configure render views.  TODO: read from file.
	if (!ConfigureRenderviews(Config)) {
		LOG_ERROR("Failed to configure renderer views. Aborting application.");
		return false;
	}

	return true;
}

bool GameInitialize(SGame* game_instance) {
	LOG_DEBUG("GameInitialize() called.");

	SGameState* State = (SGameState*)game_instance->state;

	// Load python script
	State->TestPython.SetPythonFile("recompile_shader");

	State->WorldCamera = CameraSystem::GetDefault();
	State->WorldCamera->SetPosition(Vec3(0.0f, 0.0f, -40.0f));

	// Create test ui text objects.
	if (!State->TestText.Create(Renderer, UITextType::eUI_Text_Type_Bitmap, "Ubuntu Mono 21px", 21, "Test! \n Yooo!")) {
		LOG_ERROR("Failed to load basic ui bitmap text.");
		return false;
	}
	State->TestText.SetPosition(Vec3(100, 200, 0));

	if (!State->TestSysText.Create(Renderer, UITextType::eUI_Text_Type_system, "Noto Sans CJK JP", 31, "Test system font.")) {
		LOG_ERROR("Failed to load basic ui system text.");
		return false;
	}
	State->TestSysText.SetPosition(Vec3(100, 400, 0));

	// Skybox
	if (!State->SB.Create("SkyboxCube", Renderer)) {
		LOG_ERROR("Failed to create skybox. Exiting...");
		return false;
	}

	// World meshes
	State->Meshes.resize(10);
	State->UIMeshes.resize(10);
	for (uint32_t i = 0; i < 10; ++i) {
		State->Meshes[i].Generation = INVALID_ID_U8;
		State->UIMeshes[i].Generation = INVALID_ID_U8;
	}

	Mesh* CubeMesh = &State->Meshes[0];
	CubeMesh->geometry_count = 1;
	CubeMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.World");
	CubeMesh->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig, true);
	CubeMesh->Generation = 0;
	CubeMesh->UniqueID = Identifier::AcquireNewID(CubeMesh);
	CubeMesh->Transform = Transform();

	Mesh* CubeMesh2 = &State->Meshes[1];
	CubeMesh2->geometry_count = 1;
	CubeMesh2->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh2->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig2 = GeometrySystem::GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "TestCube2", "Material.World");
	CubeMesh2->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig2, true);
	CubeMesh2->Transform = Transform(Vec3(10.0f, 0.0f, 1.0f));
	CubeMesh2->Generation = 0;
	CubeMesh2->UniqueID = Identifier::AcquireNewID(CubeMesh2);
	CubeMesh2->Transform.SetParentTransform(&CubeMesh->Transform);

	Mesh* CubeMesh3 = &State->Meshes[2];
	CubeMesh3->geometry_count = 1;
	CubeMesh3->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh3->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig3 = GeometrySystem::GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "TestCube3", "Material.World");
	CubeMesh3->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig3, true);
	CubeMesh3->Transform = Transform(Vec3(5.0f, 0.0f, 1.0f));
	CubeMesh3->Generation = 0;
	CubeMesh3->UniqueID = Identifier::AcquireNewID(CubeMesh3);
	CubeMesh3->Transform.SetParentTransform(&CubeMesh2->Transform);

	// Clean up the allocations for the geometry config.
	GeometrySystem::ConfigDispose(&GeoConfig);
	GeometrySystem::ConfigDispose(&GeoConfig2);
	GeometrySystem::ConfigDispose(&GeoConfig3);

	State->CarMesh = &State->Meshes[3];
	State->CarMesh->Transform = Transform(Vec3(15.0f, 0.0f, 1.0f));
	State->CarMesh->UniqueID = Identifier::AcquireNewID(State->CarMesh);

	State->SponzaMesh = &State->Meshes[4];
	State->SponzaMesh->Transform = Transform(Vec3(0.0f, -10.0f, 0.0f), Quaternion(Vec3(0.0f, 90.0f, 0.0f)), Vec3(0.1f, 0.1f, 0.1f));
	State->SponzaMesh->UniqueID = Identifier::AcquireNewID(State->SponzaMesh);

	// Load up some test UI geometry.
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

	const float h = game_instance->app_config.start_height / 3.0f;
	const float w = h * 200.0f / 470.0f;
	const float x = 0.0f;
	const float y = 0.0f;

	Vertex2D UIVerts[4];
	UIVerts[0].position.x = x;
	UIVerts[0].position.y = y;
	UIVerts[0].texcoord.x = 0.0f;
	UIVerts[0].texcoord.y = 1.0f;

	UIVerts[1].position.x = x + h;
	UIVerts[1].position.y = y + w;
	UIVerts[1].texcoord.x = 1.0f;
	UIVerts[1].texcoord.y = 0.0f;

	UIVerts[2].position.x = x;
	UIVerts[2].position.y = y + w;
	UIVerts[2].texcoord.x = 0.0f;
	UIVerts[2].texcoord.y = 0.0f;

	UIVerts[3].position.x = x + h;
	UIVerts[3].position.y = y;
	UIVerts[3].texcoord.x = 1.0f;
	UIVerts[3].texcoord.y = 1.0f;

	UIConfig.vertices = UIVerts;

	// Indices
	uint32_t UIIndices[6] = { 0, 2, 1, 0, 1, 3 };
	UIConfig.indices = UIIndices;

	// Get UI geometry from config.
	Mesh* UIMesh = &State->UIMeshes[0];
	UIMesh->geometry_count = 1;
	UIMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*), MemoryType::eMemory_Type_Array);
	UIMesh->geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
	UIMesh->Generation = 0;
	UIMesh->UniqueID = Identifier::AcquireNewID(UIMesh);
	UIMesh->Transform = Transform();

	// TODO: TEMP
	Core::EventRegister(Core::eEvent_Code_Debug_0, game_instance, GameOnDebugEvent);
	Core::EventRegister(Core::eEvent_Code_Object_Hover_ID_Changed, game_instance, GameOnEvent);
	// TEMP

	Core::EventRegister(Core::eEvent_Code_Key_Pressed, game_instance, GameOnKey);
	Core::EventRegister(Core::eEvent_Code_Key_Released, game_instance, GameOnKey);

	return true;
}

void GameShutdown(SGame* gameInstance) {
	SGameState* State = (SGameState*)gameInstance->state;

	// TODO: Temp
	State->SB.Destroy();

	State->TestText.Destroy();
	State->TestSysText.Destroy();

	// TODO: TEMP
	Core::EventUnregister(Core::eEvent_Code_Debug_0, gameInstance, GameOnDebugEvent);
	Core::EventUnregister(Core::eEvent_Code_Object_Hover_ID_Changed, gameInstance, GameOnEvent);
	// TEMP

	Core::EventUnregister(Core::eEvent_Code_Key_Pressed, gameInstance, GameOnKey);
	Core::EventUnregister(Core::eEvent_Code_Key_Released, gameInstance, GameOnKey);
}

bool GameUpdate(SGame* game_instance, float delta_time) {
	static size_t AllocCount = 0;
	size_t PrevAllocCount = AllocCount;
	AllocCount = Memory::GetAllocateCount();
	if (Core::InputIsKeyUp(eKeys_M) && Core::InputWasKeyDown(eKeys_M)) {
		char* Usage = Memory::GetMemoryUsageStr();
		LOG_INFO(Usage);
		StringFree(Usage);
		LOG_DEBUG("Allocations: %llu (%llu this frame)", AllocCount, AllocCount - PrevAllocCount);
	}

	SGameState* State = (SGameState*)game_instance->state;

	if (Core::InputIsKeyDown(eKeys_Left)) {
		State->WorldCamera->RotateYaw(-1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(eKeys_Right)) {
		State->WorldCamera->RotateYaw(1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Up)) {
		State->WorldCamera->RotatePitch(-1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Down)) {
		State->WorldCamera->RotatePitch(1.0f * delta_time);
	}

	float TempMoveSpeed = 50.0f;
	if (Core::InputIsKeyDown(Keys::eKeys_Shift)) {
		TempMoveSpeed *= 2.0f;
	}

	if (Core::InputIsKeyDown(Keys::eKeys_W)) {
		State->WorldCamera->MoveForward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_S)) {
		State->WorldCamera->MoveBackward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_A)) {
		State->WorldCamera->MoveLeft(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_D)) {
		State->WorldCamera->MoveRight(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Q)) {
		State->WorldCamera->MoveDown(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_E)) {
		State->WorldCamera->MoveUp(TempMoveSpeed * delta_time);
	}

	if (Core::InputIsKeyDown(Keys::eKeys_R)) {
		State->WorldCamera->Reset();
	}

	if (Core::InputIsKeyUp(eKeys_O) && Core::InputWasKeyDown(eKeys_O)) {
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Debug_0, game_instance, Context);
	}

	// TODO: Remove
	if (Core::InputIsKeyUp(eKeys_T) && Core::InputWasKeyDown(eKeys_T)) {
		State->TestPython.ExecuteFunc("CompileShaders", "glsl");
	}

	int px, py, cx, cy;
	Core::InputGetMousePosition(cx, cy);
	Core::InputGetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	if (Core::InputeIsButtonDown(eButton_Right)) {
		if (cx != px) {
			State->WorldCamera->RotateYaw((cx - px) * MouseMoveSpeed);
		}
		if (cy != py) {
			State->WorldCamera->RotatePitch((cy - py) * MouseMoveSpeed);
		}
	}

	Quaternion Rotation = QuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f * (float)delta_time, false);
	State->Meshes[0].Transform.Rotate(Rotation);
	State->Meshes[1].Transform.Rotate(Rotation);
	State->Meshes[2].Transform.Rotate(Rotation);

	// Text
	Camera* WorldCamera = CameraSystem::GetDefault();
	Vec3 Pos = WorldCamera->GetPosition();
	Vec3 Rot = WorldCamera->GetEulerAngles();

	// Mouse state
	bool LeftDown = Core::InputeIsButtonDown(eButton_Left);
	bool RightDown = Core::InputeIsButtonDown(eButton_Right);
	int MouseX, MouseY;
	Core::InputGetMousePosition(MouseX, MouseY);

	// Convert to NDC.
	float MouseX_NDC = RangeConvertfloat((float)MouseX, 0.0f, (float)State->Width, -1.0f, 1.0f);
	float MouseY_NDC = RangeConvertfloat((float)MouseY, 0.0f, (float)State->Height, -1.0f, 1.0f);

	double FPS, FrameTime;
	Metrics::Frame(&FPS, &FrameTime);

	// TODO: Temp
	char FPSText[512];
	StringFormat(FPSText, 512,
		"\
	Camera Pos: [%.3f %.3f %.3f]\n\
	Camera Rot: [%.3f %.3f %.3f]\n\
	L=%s R=%s\tNDC: x=%.2f, y=%.2f\n\
	Hovered: %s%u\n\
	FPS: %d\tDelta time: %.2f",
		Pos.x, Pos.y, Pos.z,
		Rad2Deg(Rot.x), Rad2Deg(Rot.y), Rad2Deg(Rot.z),
		LeftDown ? "Y" : "N", RightDown ? "Y" : "N",
		MouseX_NDC, MouseY_NDC,
		State->HoveredObjectID == INVALID_ID ? "None" : "",
		State->HoveredObjectID == INVALID_ID ? 0 : State->HoveredObjectID,
		(int)FPS,
		(float)FrameTime
	);
	State->TestText.SetText(FPSText);

	return true;
}

bool GameRender(SGame* game_instance, SRenderPacket* packet, float delta_time) {
	SGameState* State = (SGameState*)game_instance->state;

	// TODO: Read from config.
	packet->view_count = 4;
	std::vector<RenderViewPacket> Views;
	Views.resize(packet->view_count);
	packet->views = Views;

	// Skybox
	SkyboxPacketData SkyboxData;
	SkyboxData.sb = &State->SB;
	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("Skybox"), &SkyboxData, &packet->views[0])) {
		LOG_ERROR("Failed to build packet for view 'World_Opaque'.");
		return false;
	}

	// World
	uint32_t MeshCount = 0;
	Mesh** Meshes = (Mesh**)Memory::Allocate(sizeof(Mesh*) * 10, MemoryType::eMemory_Type_Array);
	// TODO: Flexible size array.
	for (uint32_t i = 0; i < 10; ++i) {
		if (State->Meshes[i].Generation != INVALID_ID_U8) {
			Mesh* M = &State->Meshes[i];
			Meshes[MeshCount] = M;
			MeshCount++;
		}
	}

	MeshPacketData WorldMeshData;
	WorldMeshData.meshes = Meshes;
	WorldMeshData.mesh_count = MeshCount;
	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("World"), &WorldMeshData, &packet->views[1])) {
		LOG_ERROR("Failed to build packet for view 'World'.");
		return false;
	}

	// UI
	uint32_t UIMeshCount = 0;
	Mesh** UIMeshes = (Mesh**)Memory::Allocate(sizeof(Mesh*) * 10, MemoryType::eMemory_Type_Array);
	// TODO: Flexible size array.
	for (uint32_t i = 0; i < 10; ++i) {
		if (State->UIMeshes[i].Generation != INVALID_ID_U8) {
			UIMeshes[UIMeshCount] = &State->UIMeshes[i];
			UIMeshCount++;
		}
	}


	UIText** Texts = (UIText**)Memory::Allocate(sizeof(UIText*) * 2, MemoryType::eMemory_Type_Array);
	Texts[0] = &State->TestText;
	Texts[1] = &State->TestSysText;

	UIPacketData UIPacket;
	UIPacket.meshData.mesh_count = UIMeshCount;
	UIPacket.meshData.meshes = UIMeshes;
	UIPacket.textCount = 2;
	UIPacket.Textes = Texts;

	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("UI"), &UIPacket, &packet->views[2])) {
		LOG_ERROR("Failed to build packet for view 'UI'.");
		return false;
	}

	// Pick uses both world and ui packet data.
	PickPacketData PickPacket;
	PickPacket.UIMeshData = UIPacket.meshData;
	PickPacket.WorldMeshData = WorldMeshData;
	PickPacket.Texts = UIPacket.Textes;
	PickPacket.TextCount = UIPacket.textCount;

	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("Pick"), &PickPacket, &packet->views[3])) {
		LOG_ERROR("Failed to build packet for view 'Pick'.");
		return false;
	}

	return true;
}

void GameOnResize(SGame* game_instance, unsigned int width, unsigned int height) {
	SGameState* State = (SGameState*)game_instance->state;

	State->Width = width;
	State->Height = height;

	// TODO: Temp
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

	const float h = State->Height / 3.0f;
	const float w = h * 200.0f / 470.0f;
	const float x = 0.0f;
	const float y = 0.0f;

	Vertex2D UIVerts[4];
	UIVerts[0].position.x = x;
	UIVerts[0].position.y = y;
	UIVerts[0].texcoord.x = 0.0f;
	UIVerts[0].texcoord.y = 1.0f;

	UIVerts[1].position.x = x + h;
	UIVerts[1].position.y = y + w;
	UIVerts[1].texcoord.x = 1.0f;
	UIVerts[1].texcoord.y = 0.0f;

	UIVerts[2].position.x = x;
	UIVerts[2].position.y = y + w;
	UIVerts[2].texcoord.x = 0.0f;
	UIVerts[2].texcoord.y = 0.0f;

	UIVerts[3].position.x = x + h;
	UIVerts[3].position.y = y;
	UIVerts[3].texcoord.x = 1.0f;
	UIVerts[3].texcoord.y = 1.0f;

	UIConfig.vertices = UIVerts;

	// Indices
	uint32_t UIIndices[6] = { 0, 2, 1, 0, 1, 3 };
	UIConfig.indices = UIIndices;

	State->UIMeshes[0].geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
}

bool ConfigureRenderviews(SApplicationConfig* config) {
	RenderViewConfig SkyboxConfig;
	SkyboxConfig.type = RenderViewKnownType::eRender_View_Known_Type_Skybox;
	SkyboxConfig.width = 0;
	SkyboxConfig.height = 0;
	SkyboxConfig.name = "Skybox";
	SkyboxConfig.pass_count = 1;
	SkyboxConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;

	// Renderpass config.
	std::vector<RenderpassConfig> SkyboxPasses(1);
	SkyboxPasses[0].name = "Renderpass.Builtin.Skybox";
	SkyboxPasses[0].render_area = Vec4(0, 0, 1280, 720);
	SkyboxPasses[0].clear_color = Vec4(0, 0, 0.2f, 1.0f);
	SkyboxPasses[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Color_Buffer;
	SkyboxPasses[0].depth = 1.0f;
	SkyboxPasses[0].stencil = 0;

	RenderTargetAttachmentConfig SkyboxTargetAttachment;
	// Color attachment.
	SkyboxTargetAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	SkyboxTargetAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	SkyboxTargetAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare;
	SkyboxTargetAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	SkyboxTargetAttachment.presentAfter = false;

	SkyboxPasses[0].target.attachmentCount = 1;
	SkyboxPasses[0].target.attachments.push_back(SkyboxTargetAttachment);
	SkyboxPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();

	SkyboxConfig.passes = SkyboxPasses;
	SkyboxConfig.pass_count = (unsigned char)SkyboxPasses.size();
	config->Renderviews.push_back(SkyboxConfig);

	// World view
	RenderViewConfig WorldViewConfig;
	WorldViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_World;
	WorldViewConfig.width = 0;
	WorldViewConfig.height = 0;
	WorldViewConfig.name = "World";
	WorldViewConfig.pass_count = 1;
	WorldViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;

	// Renderpass config.
	std::vector<RenderpassConfig> WorldPasses(1);
	WorldPasses[0].name = "Renderpass.Builtin.World";
	WorldPasses[0].render_area = Vec4(0, 0, 1280, 720);
	WorldPasses[0].clear_color = Vec4(0, 0.2f, 0, 1.0f);
	WorldPasses[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Stencil_Buffer | RenderpassClearFlags::eRenderpass_Clear_Depth_Buffer;
	WorldPasses[0].depth = 1.0f;
	WorldPasses[0].stencil = 0;

	RenderTargetAttachmentConfig WorldTargetColorAttachments;
	WorldTargetColorAttachments.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	WorldTargetColorAttachments.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	WorldTargetColorAttachments.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	WorldTargetColorAttachments.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	WorldTargetColorAttachments.presentAfter = false;
	WorldPasses[0].target.attachments.push_back(WorldTargetColorAttachments);

	RenderTargetAttachmentConfig WorldTargetDepthAttachments;
	WorldTargetDepthAttachments.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth;
	WorldTargetDepthAttachments.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	WorldTargetDepthAttachments.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare;
	WorldTargetDepthAttachments.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	WorldTargetDepthAttachments.presentAfter = false;
	WorldPasses[0].target.attachments.push_back(WorldTargetDepthAttachments);

	WorldPasses[0].target.attachmentCount = 2;
	WorldPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();

	WorldViewConfig.passes = WorldPasses;
	WorldViewConfig.pass_count = (unsigned char)WorldPasses.size();
	config->Renderviews.push_back(WorldViewConfig);

	// UI view
	RenderViewConfig UIViewConfig;
	UIViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_UI;
	UIViewConfig.width = 0;
	UIViewConfig.height = 0;
	UIViewConfig.name = "UI";
	UIViewConfig.pass_count = 1;
	UIViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;

	// Renderpass config
	std::vector<RenderpassConfig> UIPasses(1);
	UIPasses[0].name = "Renderpass.Builtin.UI";
	UIPasses[0].render_area = Vec4(0, 0, 1280, 720);
	UIPasses[0].clear_color = Vec4(0, 0, 0.2f, 1.0f);
	UIPasses[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_None;
	UIPasses[0].depth = 1.0f;
	UIPasses[0].stencil = 0;

	RenderTargetAttachmentConfig UITargetAttachment;
	// Color attachment.
	UITargetAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	UITargetAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	UITargetAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	UITargetAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	UITargetAttachment.presentAfter = true;

	UIPasses[0].target.attachmentCount = 1;
	UIPasses[0].target.attachments.push_back(UITargetAttachment);
	UIPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();

	UIViewConfig.passes = UIPasses;
	UIViewConfig.pass_count = (unsigned char)UIPasses.size();
	config->Renderviews.push_back(UIViewConfig);

	// Pick pass
	RenderViewConfig PickViewConfig;
	PickViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_Pick;
	PickViewConfig.width = 0;
	PickViewConfig.height = 0;
	PickViewConfig.name = "Pick";
	PickViewConfig.pass_count = 2;
	PickViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	
	// Renderpass config.
	std::vector<RenderpassConfig>PickPasses(2);
	// World pick pass
	PickPasses[0].name = "Renderpass.Builtin.WorldPick";
	PickPasses[0].render_area = Vec4(0, 0, 1280, 720);
	PickPasses[0].clear_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	PickPasses[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Color_Buffer | RenderpassClearFlags::eRenderpass_Clear_Depth_Buffer;
	PickPasses[0].depth = 1.0f;
	PickPasses[0].stencil = 0;

	RenderTargetAttachmentConfig WorldPickTargetColorAttachments;
	WorldPickTargetColorAttachments.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	WorldPickTargetColorAttachments.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	WorldPickTargetColorAttachments.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare;
	WorldPickTargetColorAttachments.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	WorldPickTargetColorAttachments.presentAfter = false;
	PickPasses[0].target.attachments.push_back(WorldPickTargetColorAttachments);

	RenderTargetAttachmentConfig WorldPickTargetDepthAttachments;
	WorldPickTargetDepthAttachments.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth;
	WorldPickTargetDepthAttachments.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	WorldPickTargetDepthAttachments.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_DontCare;
	WorldPickTargetDepthAttachments.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	WorldPickTargetDepthAttachments.presentAfter = false;
	PickPasses[0].target.attachments.push_back(WorldPickTargetDepthAttachments);

	PickPasses[0].target.attachmentCount = 2;
	PickPasses[0].renderTargetCount = 1;

	// UI pick pass
	PickPasses[1].name = "Renderpass.Builtin.UIPick";
	PickPasses[1].render_area = Vec4(0, 0, 1280, 720);
	PickPasses[1].clear_color = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	PickPasses[1].clear_flags = RenderpassClearFlags::eRenderpass_Clear_None;
	PickPasses[1].depth = 1.0f;
	PickPasses[1].stencil = 0;

	RenderTargetAttachmentConfig UIPickTargetColorAttachments;
	UIPickTargetColorAttachments.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	UIPickTargetColorAttachments.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	UIPickTargetColorAttachments.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	UIPickTargetColorAttachments.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	UIPickTargetColorAttachments.presentAfter = false;
	PickPasses[1].target.attachments.push_back(UIPickTargetColorAttachments);

	PickPasses[1].target.attachmentCount = 1;
	PickPasses[1].renderTargetCount = 1;

	PickViewConfig.passes = PickPasses;
	PickViewConfig.pass_count = (unsigned char)PickPasses.size();
	config->Renderviews.push_back(PickViewConfig);

	return true;
}
