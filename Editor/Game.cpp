#include "Game.hpp"

#include <Core/EngineLogger.hpp>
#include <Core/Controller.hpp>
#include <Core/Event.hpp>
#include <Core/Metrics.hpp>
#include "Utils/YAMLReader.h"
#include <Systems/CameraSystem.h>
#include <Containers/TString.hpp>

// TODO: Temp
#include <Systems/GeometrySystem.h>
#include <Systems/TextureSystem.h>
#include <Systems/ShaderSystem.h>
#include <Systems/RenderViewSystem.hpp>
#include <Core/Identifier.hpp>
#include <Renderer/RendererFrontend.hpp>
#include "Keybinds.hpp"
#include "GameCommands.hpp"
#include "Math/ForwardDeclarations.hpp"

static FrustumCullMode CullMode = FrustumCullMode::eAABB_Cull;
static bool EnableFrustumCulling = true;

bool GameOnEvent(eEventCode code, void* sender, void* listender_inst, SEventContext context) {
	GameInstance* GameInst = (GameInstance*)listender_inst;

	switch (code)
	{
        case eEventCode::Object_Hover_ID_Changed: 
        {
            GameInst->HoveredObjectID = context.data.u32[0];
            return true;
        }break;
        default: return true;
    }

	return false;
}

void LoadScene1(GameInstance* Game);
void LoadScene2(GameInstance* Game);
void LoadScene3(GameInstance* Game);
void LoadScene4(GameInstance* Game);

bool GameOnDebugEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
	GameInstance* GameInst = (GameInstance*)listener_instance;

	if (code == eEventCode::Debug_0) {
		LoadScene1(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_1) {
		LoadScene2(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_2) {
		LoadScene3(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_3) {
		LoadScene4(GameInst);
		return true;
	}

	return false;
}

bool GameInstance::Boot(IRenderer* renderer) {
	GLOG(Log::eInfo, "Booting...");

	YAMLReader YamlReader(EDITOR_CONFIG_PATH);
	WindowSize.Width = YamlReader.ReadPropertyInt("Window.Width");
	WindowSize.Height = YamlReader.ReadPropertyInt("Window.Height");

	GameConsole = NewObject<DebugConsoleActor>();

	Keybind GameKeybind;
	GameKeybind.Setup(this);
	GameCommand GameCmd;
	GameCmd.Setup();

	// Configure fonts.
	BitmapFontConfig BmpFontConfig;
	BmpFontConfig.name = "Ubuntu Mono 21px";
	BmpFontConfig.resourceName = "UbuntuMono21px";
	BmpFontConfig.size = 21;

	SystemFontConfig SysFontConfig;
	SysFontConfig.defaultSize = 20;
	SysFontConfig.name = "Noto Sans";
	SysFontConfig.resourceName = "NotoSansCJK";

	FontConfig.autoRelease = false;
	FontConfig.defaultBitmapFontCount = 1;
	FontConfig.bitmapFontConfigs = (BitmapFontConfig*)Memory::Allocate(sizeof(BitmapFontConfig) * 1, MemoryType::eMemory_Type_Array);
	new (static_cast<BitmapFontConfig*>(FontConfig.bitmapFontConfigs)) BitmapFontConfig(BmpFontConfig);
	FontConfig.defaultSystemFontCount = 1;
	FontConfig.systemFontConfigs = (SystemFontConfig*)Memory::Allocate(sizeof(SystemFontConfig) * 1, MemoryType::eMemory_Type_Array);
	new (static_cast<SystemFontConfig*>(FontConfig.systemFontConfigs)) SystemFontConfig(SysFontConfig);
	FontConfig.maxBitmapFontCount = 100;
	FontConfig.maxSystemFontCount = 100;

	// Configure render views.  TODO: read from file.
	if (!ConfigureRenderviews()) {
		GLOG(Log::eError, "Failed to configure renderer views. Aborting application.");
		return false;
	}

	return true;
}

bool GameInstance::Initialize() {
	GLOG(Log::eDebug, "GameInitialize() called.");
	YAMLReader YamlReader(EDITOR_CONFIG_PATH);
	Matrix4 Mat = YamlReader.ReadPropertyMatrix("Camera.Transform");

	// Load python script
	TestPython.SetPythonFile("recompile_shader");

	Vector3 Position = YamlReader.ReadPropertyVector("Camera.Position");
	Vector3 Rotation = YamlReader.ReadPropertyVector("Camera.Rotation");

	WorldCamera = CameraSystem::GetDefault();
	WorldCamera->SetPosition(Position);
	WorldCamera->SetEulerAngles(Rotation);

	// Create test ui text objects.
	if (!TestText.Create(UITextType::eUI_Text_Type_Bitmap, "Ubuntu Mono 21px", 21, "Test! \n Yooo!")) {
		GLOG(Log::eError, "Failed to load basic ui bitmap text.");
		return false;
	}
	TestText.SetLocation(Vector3(150, 450, 0));
	TestText.SetName("Render information window.");

	if (!TestSysText.Create(UITextType::eUI_Text_Type_system, 
		"Noto Sans CJK JP", 25, "Keyboard map:\
		\nLoad models:\
		\n\tO: Scene1 P: Scene2\
		\n\tK: Scene3 L: Scene4\
		\nM: Watch memory usage.\
		\nF1: Physics Based Render Render.\
		\nF2: Blinn-Phong.\
		\nF3: Light view.\
		\nF4: Depth view."))
	{
		GLOG(Log::eError, "Failed to load basic ui system text.");
		return false;
	}
	TestSysText.SetLocation(Vector3(100, 200, 0));
	TestSysText.SetName("Keyboard map texts.");

	// Load console
	GameConsole->Initialize();

	// Skybox
	if (!SB.Create("SkyboxCube")) {
		GLOG(Log::eError, "Failed to create skybox. Exiting...");
		return false;
	}

	// World meshes
	Mesh* CubeMesh = NewObject<Mesh>();
	CubeMesh->Name = "TestCube";
	CubeMesh->geometry_count = 1;
	CubeMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.Builtin.GBuffer");
	CubeMesh->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig, true);
	CubeMesh->Generation = 0;
	CubeMesh->SetTransform(Vector(0.0f, 0.0f, 0.0f), Quaternion(Vector(0.0f, 0.0f, 0.0f)));
	Meshes.Push(CubeMesh);

	Mesh* CubeMesh2 = NewObject<Mesh>();
	CubeMesh2->Name = "TestCube2";
	CubeMesh2->geometry_count = 1;
	CubeMesh2->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh2->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig2 = GeometrySystem::GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "TestCube2", "Material.Builtin.GBuffer");
	CubeMesh2->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig2, true);
	CubeMesh2->SetTransform(Vector3(10.0f, 0.0f, 1.0f), Quaternion(Vector(0.0f, 0.0f, 0.0f)));
	CubeMesh2->Generation = 0;
	CubeMesh2->AttachTo(CubeMesh);
	Meshes.Push(CubeMesh2);

	Mesh* CubeMesh3 = NewObject<Mesh>();
	CubeMesh3->Name = "TestCube3";
	CubeMesh3->geometry_count = 1;
	CubeMesh3->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh3->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig3 = GeometrySystem::GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "TestCube3", "Material.Builtin.GBuffer");
	CubeMesh3->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig3, true);
	CubeMesh3->SetTransform(Vector3(5.0f, 0.0f, 1.0f));
	CubeMesh3->Generation = 0;
	CubeMesh3->AttachTo(CubeMesh2);
	Meshes.Push(CubeMesh3);

	// Clean up the allocations for the geometry config.
	GeometrySystem::ConfigDispose(&GeoConfig);
	GeometrySystem::ConfigDispose(&GeoConfig2);
	GeometrySystem::ConfigDispose(&GeoConfig3);

	// Load up some test UI geometry.
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	UIConfig.material_name = "Material.UI";
	UIConfig.name = "Material.UI";

	const float h = WindowSize.Height / 3.0f;
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
	Mesh* UIMesh = NewObject<Mesh>();
	UIMesh->Name = "Engine Logo UI";
	UIMesh->geometry_count = 1;
	UIMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*), MemoryType::eMemory_Type_Array);
	UIMesh->geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
	UIMesh->Generation = 0;
	UIMeshes.Push(UIMesh);

	// TODO: TEMP
	EngineEvent::Register(eEventCode::Debug_0, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_1, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_2, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_3, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Object_Hover_ID_Changed, this, GameOnEvent);
	// TEMP

	return true;
}

void GameInstance::Shutdown() {
	// TODO: Temp
	SB.Destroy();

	if (GameConsole) {
		DeleteObject(GameConsole);
	}

	// Delete meshes.
	for (Mesh* m : Meshes) {
		if (m) {
			DeleteObject(m);
		}
	}

	TestText.Destroy();
	TestSysText.Destroy();

	YAMLReader YamlReader(EDITOR_CONFIG_PATH);
	YamlReader.SetPropertyInt("Window.Width", WindowSize.Width);
	YamlReader.SetPropertyInt("Window.Height", WindowSize.Height);
	YamlReader.SetPropertyVector("Camera.Position", WorldCamera->GetPosition());
	YamlReader.SetPropertyVector("Camera.Rotation", WorldCamera->GetEulerAngles());

	// TODO: TEMP
	EngineEvent::Unregister(eEventCode::Debug_0, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_1, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_2, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_3, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Object_Hover_ID_Changed, this, GameOnEvent);
	// TEMP
}

bool GameInstance::Update(float delta_time) {
	for (int i = 0; i < Meshes.Size(); ++i) {
		Meshes[i]->Tick(delta_time);
	}

	for(int i = 0; i < UIMeshes.Size(); ++i) {
		UIMeshes[i]->Tick(delta_time);
	}

	TestText.Tick(delta_time);
	TestSysText.Tick(delta_time);

	// Ensure this is cleaned up to avoid leaking memory.
	// TODO: Need a version of this that uses the frame allocator.
	if (!FrameData.WorldGeometries.empty()) {
		FrameData.WorldGeometries.clear();
		std::vector<GeometryRenderData>().swap(FrameData.WorldGeometries);
	}

	int px, py, cx, cy;
	Controller::GetMousePosition(cx, cy);
	Controller::GetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	if (Controller::IsButtonDown(eButtons::Right)) {
		if (cx != px) {
			WorldCamera->RotateYaw((px - cx) * MouseMoveSpeed);
		}
		if (cy != py) {
			WorldCamera->RotatePitch((py - cy) * MouseMoveSpeed);
		}
	}

	Quaternion RotationY = Quaternion(Axis::Y, 0.5f * (float)delta_time, false);
	Quaternion RotationX = Quaternion(Axis::X, 0.5f * (float)delta_time, false);
	Meshes[0]->Rotate(RotationY);
	Meshes[1]->Rotate(RotationY);
	Meshes[2]->Rotate(RotationY);

	// Text
	Camera* WorldCamera = CameraSystem::GetDefault();
	Vector3 Pos = WorldCamera->GetPosition();
	Vector3 Rot = WorldCamera->GetEulerAngles();

	// Mouse state
	bool LeftDown = Controller::IsButtonDown(eButtons::Left);
	bool RightDown = Controller::IsButtonDown(eButtons::Right);
	int MouseX, MouseY;
	Controller::GetMousePosition(MouseX, MouseY);

	// Convert to NDC.
	float MouseX_NDC = RangeConvertfloat((float)MouseX, 0.0f, (float)WindowSize.Width, -1.0f, 1.0f);
	float MouseY_NDC = RangeConvertfloat((float)MouseY, 0.0f, (float)WindowSize.Height, -1.0f, 1.0f);

	double FPS, FrameTime;
	Metrics::Frame(&FPS, &FrameTime);

	// Update the frustum.
	Vector3 Forward = WorldCamera->Forward();
	Vector3 Right = WorldCamera->Right();
	Vector3 Up = WorldCamera->Up();
	// TODO: Get camera fov, aspect etc.
	CameraFrustum = Frustum(WorldCamera->GetPosition(), Forward, Right, Up, 
		(float)WindowSize.Width / (float)WindowSize.Height, Deg2Rad(45.0f), 0.1f, 1000.0f);

	// NOTE: starting at a reasonable default to avoid too many realloc.
	uint32_t DrawCount = 0;
	for (uint32_t i = 0; i < (uint32_t)Meshes.Size(); ++i) {
		Mesh* m = Meshes[i];
		if (m == nullptr) {
			continue;
		}

		if (m->Generation != INVALID_ID_U8) {
			Matrix4 Model = m->GetWorldTransform();

			for (uint32_t j = 0; j < m->geometry_count; j++) {
				Geometry* g = m->geometries[j];
				if (g == nullptr) {
					continue;
				}

				switch (CullMode)
				{
				// Bounding sphere calculation
				case FrustumCullMode::eSphere_Cull:
				{
					Vector3 ExtensMin = g->Extents.min.Transform(Model);
					Vector3 ExtensMax = g->Extents.max.Transform(Model);

					float Min = DMIN(DMIN(ExtensMin.x, ExtensMin.y), ExtensMin.z);
					float Max = DMIN(DMIN(ExtensMax.x, ExtensMax.y), ExtensMax.z);
					float Diff = Dabs(Max - Min);
					float Radius = Diff / 2.0f;

					// Translate/scale the center.
					Vector3 Center = g->Center.Transform(Model);

					if (CameraFrustum.IntersectsSphere(Center, Radius)) {
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model = Model;
						Data.geometry = g;
						Data.uniqueID = m->GetUniqueID();
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
				} break;
				// AABB calculation
				case FrustumCullMode::eAABB_Cull:
				{
					// Translate/scale the extents.
					Vector3 ExtentsMax = g->Extents.max.Transform(Model);

					// Translate/scale the center.
					Vector3 Center = g->Center.Transform(Model);
					Vector3 HalfExtents = {
						Dabs(ExtentsMax.x - Center.x),
						Dabs(ExtentsMax.y - Center.y),
						Dabs(ExtentsMax.z - Center.z)
					};

					if (CameraFrustum.IntersectsAABB(Center, HalfExtents) && EnableFrustumCulling) {
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model = Model;
						Data.geometry = g;
						Data.uniqueID = m->GetUniqueID();
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
					else if (!EnableFrustumCulling) {
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model = Model;
						Data.geometry = g;
						Data.uniqueID = m->GetUniqueID();
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
				} break;
				}
			}
		}
	}


	// TODO: Temp
	std::string HoverdObjectName = "None";
	if (HoveredObjectID != INVALID_ID) {
		if (HoveredObjectID == TestText.GetUniqueID()) {
			HoverdObjectName = TestText.GetName();
		}
		if (HoveredObjectID == TestSysText.GetUniqueID()) {
			HoverdObjectName = TestSysText.GetName();
		}

		for (Mesh* Mesh : Meshes) {
			if (Mesh->GetUniqueID() == HoveredObjectID)
			{
				HoverdObjectName = Mesh->Name;
				break;
			}
		}
		for (Mesh* UI : UIMeshes) {
			if (UI->GetUniqueID() == HoveredObjectID)
			{
				HoverdObjectName = UI->Name;
				break;
			}
		}
	}

	char FPSText[512];
	StringFormat(FPSText, 512,
		"\
	Camera Pos: [%.3f %.3f %.3f]\tCamera Rot: [%.3f %.3f %.3f]\n\
	L=%s R=%s\tNDC: x=%.2f, y=%.2f\tHovered Object: %s\n\
	FPS: %d\tDelta time: %.2f\n\
	Drawn Count: %-5u",
		Pos.x, Pos.y, Pos.z,
		Rot.x, Rot.y, Rot.z,
		LeftDown ? "Y" : "N", RightDown ? "Y" : "N",
		MouseX_NDC, MouseY_NDC,
		HoverdObjectName.c_str(),
		(int)FPS,
		(float)FrameTime,
		DrawCount
	);
	TestText.SetText(FPSText);

	GameConsole->Tick(delta_time);

	return true;
}

static float GameTime = 0.0f;

bool GameInstance::Render(SRenderPacket* packet, float delta_time) {
	GameTime += delta_time;

	// TODO: Read from config.
	packet->view_count = 4;
	packet->views.resize(packet->view_count);
	uint32_t ViewCounter = 0;

	// Skybox
	SkyboxPacketData SkyboxData;
	SkyboxData.sb = &SB;
	IRenderView* SkyboxView = RenderViewSystem::Get("Skybox");
	if (SkyboxView) {
		if (!RenderViewSystem::BuildPacket(SkyboxView, &SkyboxData, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'World_Opaque'.");
			return false;
		}
	}

	// World
	IRenderView* WorldView = RenderViewSystem::Get("WorldDeferred");
	if(WorldView) {
		WorldPacketData WorldData;
		WorldData.Meshes = FrameData.WorldGeometries;
		WorldData.GlobalTime = GameTime;
		if (!RenderViewSystem::BuildPacket(WorldView, &WorldData, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'World'.");
			return false;
		}
	}
	
	// UI
	uint32_t UIMeshCount = 0;
	Mesh** TempUIMeshes = (Mesh**)Memory::Allocate(sizeof(Mesh*) * 10, MemoryType::eMemory_Type_Array);
	// TODO: Flexible size array.
	for (uint32_t i = 0; i < (uint32_t)UIMeshes.Size(); ++i) {
		if (UIMeshes[i]->Generation != INVALID_ID_U8) {
			TempUIMeshes[UIMeshCount] = UIMeshes[i];
			UIMeshCount++;
		}
	}

	UIText** Texts = (UIText**)Memory::Allocate(sizeof(UIText*) * 4, MemoryType::eMemory_Type_Array);
	Texts[0] = &TestText;
	Texts[1] = &TestSysText;
	Texts[2] = GameConsole->GetText();
	Texts[3] = GameConsole->GetEntryText();

	UIPacketData UIPacket;
	UIPacket.meshData.mesh_count = UIMeshCount;
	UIPacket.meshData.meshes = TempUIMeshes;
	UIPacket.textCount = 4;
	UIPacket.Textes = Texts;

	IRenderView* UIView = RenderViewSystem::Get("UI");
	if (UIView) {
		if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("UI"), &UIPacket, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'UI'.");
			return false;
		}
	}
	
	IRenderView* PickView = RenderViewSystem::Get("Pick");
	if (PickView) {
		// Pick uses both world and ui packet data.
		PickPacketData PickPacket;
		PickPacket.UIMeshData = UIPacket.meshData;
		PickPacket.WorldMeshData = FrameData.WorldGeometries;
		PickPacket.Texts = UIPacket.Textes;
		PickPacket.TextCount = UIPacket.textCount;

		if (!RenderViewSystem::BuildPacket(PickView, &PickPacket, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'Pick'.");
			return false;
		}
	}

	return true;
}

void GameInstance::OnResize(unsigned int width, unsigned int height) {
	WindowSize = { (int)width, (int)height };

	TestText.SetLocation(Vector3(180, (float)height - 150, 0));
	TestSysText.SetLocation(Vector3(100, (float)height - 400, 0));

	// TODO: Temp
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	UIConfig.material_name = "Material.UI";
	UIConfig.name = "Material.UI";

	const float h = WindowSize.Height / 3.0f;
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

	UIMeshes[0]->geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
}

bool GameInstance::ConfigureRenderviews() {
	IRenderer* Renderer = IRenderer::GetRenderer();

	RenderViewConfig SkyboxConfig;
	SkyboxConfig.type = RenderViewKnownType::eRender_View_Known_Type_Skybox;
	SkyboxConfig.width = GetWindowWidth();
	SkyboxConfig.height = GetWindowHeight();
	SkyboxConfig.name = "Skybox";
	SkyboxConfig.pass_count = 1;
	SkyboxConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;

	// Renderpass config.
	std::vector<RenderpassConfig> SkyboxPasses(1);
	SkyboxPasses[0].name = "Renderpass.Builtin.Skybox";
	SkyboxPasses[0].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	SkyboxPasses[0].clear_color = Vector4(0, 0, 0.2f, 1.0f);
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
	SkyboxTargetAttachment.index = 0;

	SkyboxPasses[0].target.attachments.push_back(SkyboxTargetAttachment);
	SkyboxPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();

	SkyboxConfig.passes = SkyboxPasses;
	SkyboxConfig.pass_count = (unsigned char)SkyboxPasses.size();
	Renderviews.push_back(SkyboxConfig);

	// Deferred render
	RenderViewConfig WorldViewConfig;
	WorldViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_Deferred;
	WorldViewConfig.width = GetWindowWidth();
	WorldViewConfig.height = GetWindowHeight();
	WorldViewConfig.name = "WorldDeferred";
	WorldViewConfig.pass_count = 2;
	WorldViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	// 延迟渲染通道配置 - 需要两个通道
	std::vector<RenderpassConfig> WorldPasses(2);  // 改为2个通道

	// 第一通道：G-Buffer渲染
	Memory::Zero(&WorldPasses[0], sizeof(RenderpassConfig));

	WorldPasses[0].name = "Renderpass.Builtin.GBuffer";
	WorldPasses[0].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	WorldPasses[0].clear_color = Vector4(0.0f, 0.0f, 0.0f, 0.0f);  // 明确设置为0.0f
	WorldPasses[0].clear_flags = RenderpassClearFlags::eRenderpass_Clear_Depth_Buffer |
								 RenderpassClearFlags::eRenderpass_Clear_Stencil_Buffer;
	WorldPasses[0].depth = 1.0f;        // 深度清除值：1.0f (最远)
	WorldPasses[0].stencil = 0;         // 模板清除值：0

	// G-Buffer颜色附件0：Albedo + Metallic (RGBA8)
	RenderTargetAttachmentConfig GBufferAlbedoAttachment;
	Memory::Zero(&GBufferAlbedoAttachment, sizeof(RenderTargetAttachmentConfig));
	GBufferAlbedoAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	GBufferAlbedoAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	GBufferAlbedoAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Clear;
	GBufferAlbedoAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	GBufferAlbedoAttachment.presentAfter = false;
	GBufferAlbedoAttachment.index = 0;
	WorldPasses[0].target.attachments.push_back(GBufferAlbedoAttachment);

	// G-Buffer颜色附件1：Normal + Roughness (RGBA16F)
	RenderTargetAttachmentConfig GBufferNormalAttachment;
	Memory::Zero(&GBufferNormalAttachment, sizeof(RenderTargetAttachmentConfig));
	GBufferNormalAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	GBufferNormalAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	GBufferNormalAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Clear;
	GBufferNormalAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	GBufferNormalAttachment.presentAfter = false;
	GBufferNormalAttachment.index = 1;
	WorldPasses[0].target.attachments.push_back(GBufferNormalAttachment);

	// G-Buffer颜色附件2：Position + Depth (RGBA32F)
	RenderTargetAttachmentConfig GBufferPositionAttachment;
	Memory::Zero(&GBufferPositionAttachment, sizeof(RenderTargetAttachmentConfig));
	GBufferPositionAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	GBufferPositionAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_View;
	GBufferPositionAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Clear;
	GBufferPositionAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	GBufferPositionAttachment.presentAfter = false;
	GBufferPositionAttachment.index = 2;
	WorldPasses[0].target.attachments.push_back(GBufferPositionAttachment);

	// G-Buffer深度附件
	RenderTargetAttachmentConfig GBufferDepthAttachment;
	Memory::Zero(&GBufferDepthAttachment, sizeof(RenderTargetAttachmentConfig));
	GBufferDepthAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Depth;
	GBufferDepthAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	GBufferDepthAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Clear;
	GBufferDepthAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	GBufferDepthAttachment.presentAfter = false;
	GBufferDepthAttachment.index = 0;
	WorldPasses[0].target.attachments.push_back(GBufferDepthAttachment);

	// 设置G-Buffer通道的渲染目标数量
	WorldPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();  // 双缓冲

	// 第二通道：延迟光照
	Memory::Zero(&WorldPasses[1], sizeof(RenderpassConfig));
	WorldPasses[1].name = "Renderpass.Builtin.DeferredLighting";
	WorldPasses[1].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	WorldPasses[1].clear_color = Vector4(0.0f, 0.2f, 0.0f, 1.0f);  // 最终输出的清除色
	WorldPasses[1].clear_flags = RenderpassClearFlags::eRenderpass_Clear_None;  
	WorldPasses[1].depth = 1.0f;       
	WorldPasses[1].stencil = 0;        

	// 光照通道的颜色输出（通常是swapchain或者屏幕缓冲）
	RenderTargetAttachmentConfig LightingColorAttachment;
	Memory::Zero(&LightingColorAttachment, sizeof(RenderTargetAttachmentConfig));
	LightingColorAttachment.type = RenderTargetAttachmentType::eRender_Target_Attachment_Type_Color;
	LightingColorAttachment.source = RenderTargetAttachmentSource::eRender_Target_Attachment_Source_Default;
	LightingColorAttachment.loadOperation = RenderTargetAttachmentLoadOperation::eRender_Target_Attachment_Load_Operation_Load;
	LightingColorAttachment.storeOperation = RenderTargetAttachmentStoreOperation::eRender_Target_Attachment_Store_Operation_Store;
	LightingColorAttachment.presentAfter = false; 
	LightingColorAttachment.index = 0;
	WorldPasses[1].target.attachments.push_back(LightingColorAttachment);

	// 设置光照通道的渲染目标数量
	WorldPasses[1].renderTargetCount = Renderer->GetWindowAttachmentCount();

	// 最终配置
	WorldViewConfig.passes = WorldPasses;
	WorldViewConfig.pass_count = (unsigned char)WorldPasses.size();
	Renderviews.push_back(WorldViewConfig);

	// UI view
	RenderViewConfig UIViewConfig;
	UIViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_UI;
	UIViewConfig.width = GetWindowWidth();
	UIViewConfig.height = GetWindowHeight();
	UIViewConfig.name = "UI";
	UIViewConfig.pass_count = 1;
	UIViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;

	// Renderpass config
	std::vector<RenderpassConfig> UIPasses(1);
	UIPasses[0].name = "Renderpass.Builtin.UI";
	UIPasses[0].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	UIPasses[0].clear_color = Vector4(0, 0, 0.2f, 1.0f);
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

	UIPasses[0].target.attachments.push_back(UITargetAttachment);
	UIPasses[0].renderTargetCount = Renderer->GetWindowAttachmentCount();

	UIViewConfig.passes = UIPasses;
	UIViewConfig.pass_count = (unsigned char)UIPasses.size();
	Renderviews.push_back(UIViewConfig);

	// Pick pass
	RenderViewConfig PickViewConfig;
	PickViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_Pick;
	PickViewConfig.width = GetWindowWidth();
	PickViewConfig.height = GetWindowHeight();
	PickViewConfig.name = "Pick";
	PickViewConfig.pass_count = 2;
	PickViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	
	// Renderpass config.
	std::vector<RenderpassConfig>PickPasses(2);
	// World pick pass
	PickPasses[0].name = "Renderpass.Builtin.WorldPick";
	PickPasses[0].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	PickPasses[0].clear_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
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

	PickPasses[0].renderTargetCount = 1;

	// UI pick pass
	PickPasses[1].name = "Renderpass.Builtin.UIPick";
	PickPasses[1].render_area = Vector4(0, 0, (float)GetWindowWidth(), (float)GetWindowHeight());
	PickPasses[1].clear_color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
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

	PickPasses[1].renderTargetCount = 1;

	PickViewConfig.passes = PickPasses;
	PickViewConfig.pass_count = (unsigned char)PickPasses.size();
	Renderviews.push_back(PickViewConfig);

	return true;
}


void LoadScene1(GameInstance* GameInst) {
	for (size_t i = GameInst->Meshes.Size() - 1; i >= 3; --i) {
		Mesh* M = GameInst->Meshes[i];
		DeleteObject(M);
		GameInst->Meshes[i] = nullptr;
		GameInst->Meshes.Pop();
	}

	Mesh* Model = NewObject<Mesh>();
	Model->LoadFromResource("mountain_part");	// It always return true.
	Model->SetTransform(Vector3(300.0f, -50.0f, 0.0f), Quaternion(Vector3(0.0f, 0.0f, 0.0f)), Vector3(50.f));
	GameInst->Meshes.Push(Model);
}

void LoadScene2(GameInstance* GameInst) {
	for (size_t i = GameInst->Meshes.Size() - 1; i >= 3; --i) {
		Mesh* M = GameInst->Meshes[i];
		DeleteObject(M);
		GameInst->Meshes[i] = nullptr;
		GameInst->Meshes.Pop();
	}

	Mesh* Model1 = NewObject<Mesh>();
	Model1->LoadFromResource("sponza");	// It always return true.
	Model1->SetTransform(Vector3(0.0f, -10.0f, 0.0f), Quaternion(Vector3(0.0f, 90.0f, 0.0f)), Vector3(0.1f));
	GameInst->Meshes.Push(Model1);

	Mesh* Model2 = NewObject<Mesh>();
	Model2->LoadFromResource("bunny");	// It always return true.
	Model2->SetTransform(Vector3(30.0f, 0.0f, 0.0f), Quaternion(Vector3(0.0f, 0.0f, 0.0f)), Vector3(5.0f));
	GameInst->Meshes.Push(Model2);

	Mesh* Model3 = NewObject<Mesh>();
	Model3->LoadFromResource("falcon");	// It always return true.
	Model3->SetTransform(Vector3(-30.0f, 0.0f, 0.0f), Quaternion(Vector3(0.0f, 0.0f, 0.0f)));
	GameInst->Meshes.Push(Model3);
}

void LoadScene3(GameInstance* GameInst) {
	for (size_t i = GameInst->Meshes.Size() - 1; i >= 3; --i) {
		Mesh* M = GameInst->Meshes[i];
		DeleteObject(M);
		GameInst->Meshes[i] = nullptr;
		GameInst->Meshes.Pop();
	}

	Mesh* Model = NewObject<Mesh>();
	Model->LoadFromResource("Axis");	
	Model->SetTransform(Vector3(0.0f, 10.0f, 0.0f), Quaternion(Vector3(0.0f, 0.0f, 0.0f)), Vector3(500.f));
	GameInst->Meshes.Push(Model);
}

void LoadScene4(GameInstance* GameInst) {
	for (size_t i = GameInst->Meshes.Size() - 1; i >= 3; --i) {
		Mesh* M = GameInst->Meshes[i];
		DeleteObject(M);
		GameInst->Meshes[i] = nullptr;
		GameInst->Meshes.Pop();
	}

	Mesh* Model = NewObject<Mesh>();
	Model->LoadFromResource("LegoCar");	
	Model->SetTransform(Vector3(0.0f, 0.0f, -50.0f), Quaternion(Vector3(0.0f, 0.0f, 0.0f)), Vector3(0.5f));
	GameInst->Meshes.Push(Model);
}