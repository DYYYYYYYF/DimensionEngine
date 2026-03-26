#include "Game.h"

#include <Core/EngineLogger.hpp>
#include <Core/Controller.hpp>
#include <Core/Event.hpp>
#include <Core/Metrics.hpp>
#include <Systems/CameraSystem.h>
#include <Platform/File/JsonObject.h>
#include <Containers/FString.hpp>

// TODO: Temp
#include <Systems/GeometrySystem.h>
#include <Systems/TextureSystem.h>
#include <Systems/ShaderSystem.h>
#include <Systems/RenderViewSystem.hpp>
#include <Core/Identifier.hpp>
#include <Rendering/Renderer.hpp>
#include <Rendering/Resources/Skybox/Skybox.hpp>
#include "UI/Console/Keybinds.h"
#include "UI/Console/GameCommand.h"
#include "Math/ForwardDeclarations.hpp"
#include "Framework/Classes/StaticMeshActor.h"
#include "GameLogic/TestActors/RotationCubeActor.h"

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

	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	JsonObject Content = JsonObject(MaterialAsset.ReadText());
	WindowSize.Width = (uint16_t)Content.ReadInt("Window.Width");
	WindowSize.Height = (uint16_t)Content.ReadInt("Window.Height");

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

	return true;
}

bool GameInstance::Initialize() {
	GLOG(Log::eDebug, "GameInitialize() called.");
	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	// Get transform
	JsonObject Content = JsonObject(MaterialAsset);
	Matrix4 Mat = Content.ReadMatrix4("Camera.Transform");
	
	// Load python script
	TestPython.SetPythonFile("recompile_shader");

	Vector3 Position = Content.ReadVector3("Camera.Position");
	Vector3 Rotation = Content.ReadVector3("Camera.Rotation");

	WorldCamera = CameraSystem::Get().GetDefault();
	WorldCamera->SetPosition(Position);
	WorldCamera->SetEulerAngles(Rotation);

	// Create test ui text objects.
	TestText = NewObject<ATextActor>(UITextType::eUI_Text_Type_Bitmap, "Ubuntu Mono 21px", 21, "Test! \n Yooo!");
	if (TestText) {
		TestText->SetLocation(Vector3(150, 450, 0));
		TestText->SetName("Render information window.");
	}

	TestSysText = NewObject<ATextActor>(UITextType::eUI_Text_Type_system,
		"Noto Sans CJK JP", 25, "Keyboard map:\
		\nLoad models:\
		\n\tO: Scene1 P: Scene2\
		\n\tK: Scene3 L: Scene4\
		\nM: Watch memory usage.\
		\nF1: Default view.\
		\nF2: Normal view.\
		\nF3: Material view.\
		\nF4: Depth view.");

	if (TestSysText) {
		TestSysText->SetLocation(Vector3(100, 200, 0));
		TestSysText->SetName("Keyboard map texts.");
	}

	// Load console
	GameConsole->Initialize();

	// Skybox
	SB = NewObject<Skybox>();
	if (!SB) {
		GLOG(Log::eError, "Failed to create skybox. Exiting...");
		return false;
	}

	if (!SB->Create("SkyboxCube")) {
		GLOG(Log::eError, "Failed to create skybox. Exiting...");
		return false;
	}

	// World meshes
	ARotationCubeActor* CubeMesh = NewObject<ARotationCubeActor>("TestCube");
	UTransformComponent* TansformComp1 = CubeMesh->GetComponent<UTransformComponent>();
	TansformComp1->SetLocation(Vector(0.0f, 0.0f, 0.0f));
	TansformComp1->SetRotation(Vector(0.0f));
	Meshes.Push(CubeMesh);

	ARotationCubeActor* CubeMesh2 = NewObject<ARotationCubeActor>("TestCube2");
	UTransformComponent* TansformComp2 = CubeMesh2->GetComponent<UTransformComponent>();
	TansformComp2->SetLocation(Vector(10.0f, 0.0f, 0.0f));
	TansformComp2->SetRotation(Vector(0.0f));
	TansformComp2->SetScale(Vector(0.5f));
	CubeMesh2->AttachTo(CubeMesh);
	Meshes.Push(CubeMesh2);

	ARotationCubeActor* CubeMesh3 = NewObject<ARotationCubeActor>("TestCube3");
	UTransformComponent* TansformComp3 = CubeMesh3->GetComponent<UTransformComponent>();
	TansformComp3->SetLocation(Vector(15.0f, 0.0f, 0.0f));
	TansformComp3->SetRotation(Vector(0.0f));
	TansformComp3->SetScale(Vector(0.3f));
	CubeMesh3->AttachTo(CubeMesh2);
	Meshes.Push(CubeMesh3);

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
	AStaticMeshActor* UIMesh = NewObject<AStaticMeshActor>("Engine Logo UI");
	UIMesh->geometry_count = 1;
	UIMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*), MemoryType::eMemory_Type_Array);
	UIMesh->geometries[0] = GeometrySystem::Get().AcquireFromConfig(UIConfig, true);
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
	if (SB) {
		SB->Destroy();
		DeleteObject(SB);
	}

	if (GameConsole) {
		DeleteObject(GameConsole);
	}

	// Delete meshes.
	for (AStaticMeshActor* m : Meshes) {
		if (m) {
			DeleteObject(m);
		}
	}

	if (TestText) {
		TestText->Destroy();
		DeleteObject(TestText);
		TestText = nullptr;
	}

	if (TestSysText) {
		TestSysText->Destroy();
		DeleteObject(TestSysText);
		TestSysText = nullptr;
	}

	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return;
	}

	JsonObject Content = JsonObject(MaterialAsset);
	Content.WriteInt("Window.Width", (int)WindowSize.Width);
	Content.WriteInt("Window.Height", (int)WindowSize.Height);
	Content.WriteVector3("Camera.Position", WorldCamera->GetPosition());
	Content.WriteVector3("Camera.Rotation", WorldCamera->GetEulerAngles());
	Content.SaveToFile(MaterialAsset);

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

	TestText->Tick(delta_time);
	TestSysText->Tick(delta_time);

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

	// Text
	WorldCamera = CameraSystem::Get().GetDefault();
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
		AStaticMeshActor* m = Meshes[i];
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
						Data.model_mat = Model;
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
						Data.model_mat = Model;
						Data.geometry = g;
						Data.uniqueID = m->GetUniqueID();
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
					else if (!EnableFrustumCulling) {
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model_mat = Model;
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
		if (HoveredObjectID == TestText->GetUniqueID()) {
			HoverdObjectName = TestText->GetName().CStr();
		}
		if (HoveredObjectID == TestSysText->GetUniqueID()) {
			HoverdObjectName = TestSysText->GetName().CStr();
		}

		for (AStaticMeshActor* Mesh : Meshes) {
			if (Mesh->GetUniqueID() == HoveredObjectID)
			{
				HoverdObjectName = Mesh->GetName().CStr();
				break;
			}
		}
		for (AStaticMeshActor* UI : UIMeshes) {
			if (UI->GetUniqueID() == HoveredObjectID)
			{
				HoverdObjectName = UI->GetName().CStr();
				break;
			}
		}
	}

	FString FPSText = FString::Format("\
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
	TestText->SetContent(FPSText);

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

	RenderViewSystem& RenderviewSys = RenderViewSystem::Get();

	// Skybox
	SkyboxPacketData SkyboxData;
	SkyboxData.sb = SB;
	IRenderView* SkyboxView = RenderviewSys.Get("Skybox");
	if (SkyboxView) {
		if (!RenderviewSys.BuildPacket(SkyboxView, &SkyboxData, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'World_Opaque'.");
			return false;
		}
	}

	// World
	IRenderView* WorldView = RenderviewSys.Get("WorldDeferred");
	if(WorldView) {
		WorldPacketData WorldData;
		WorldData.Meshes = FrameData.WorldGeometries;
		WorldData.GlobalTime = GameTime;
		if (!RenderviewSys.BuildPacket(WorldView, &WorldData, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'World'.");
			return false;
		}
	}
	
	// UI
	uint32_t UIMeshCount = 0;
	AStaticMeshActor** TempUIMeshes = (AStaticMeshActor**)Memory::Allocate(sizeof(AStaticMeshActor*) * 10, MemoryType::eMemory_Type_Array);
	// TODO: Flexible size array.
	for (uint32_t i = 0; i < (uint32_t)UIMeshes.Size(); ++i) {
		if (UIMeshes[i]->Generation != INVALID_ID_U8) {
			TempUIMeshes[UIMeshCount] = UIMeshes[i];
			UIMeshCount++;
		}
	}

	ATextActor** Texts = (ATextActor**)Memory::Allocate(sizeof(ATextActor*) * 4, MemoryType::eMemory_Type_Array);
	Texts[0] = TestText;
	Texts[1] = TestSysText;
	Texts[2] = GameConsole->GetText();
	Texts[3] = GameConsole->GetEntryText();

	UIPacketData UIPacket;
	UIPacket.meshData.mesh_count = UIMeshCount;
	UIPacket.meshData.meshes = (AActor**)TempUIMeshes;
	UIPacket.textCount = 4;
	UIPacket.Textes = Texts;

	IRenderView* UIView = RenderviewSys.Get("UI");
	if (UIView) {
		if (!RenderviewSys.BuildPacket(RenderviewSys.Get("UI"), &UIPacket, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'UI'.");
			return false;
		}
	}
	
	IRenderView* PickView = RenderviewSys.Get("Pick");
	if (PickView) {
		// Pick uses both world and ui packet data.
		PickPacketData PickPacket;
		PickPacket.UIMeshData = UIPacket.meshData;
		PickPacket.WorldMeshData = FrameData.WorldGeometries;
		PickPacket.Texts = UIPacket.Textes;
		PickPacket.TextCount = UIPacket.textCount;

		if (!RenderviewSys.BuildPacket(PickView, &PickPacket, &packet->views[ViewCounter++])) {
			GLOG(Log::eError, "Failed to build packet for view 'Pick'.");
			return false;
		}
	}

	return true;
}

void GameInstance::OnResize(unsigned int width, unsigned int height) {
	WindowSize = { (uint16_t)width, (uint16_t)height };

	TestText->SetLocation(Vector3(180, (float)height - 150, 0));
	TestSysText->SetLocation(Vector3(100, (float)height - 400, 0));

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

	GeometrySystem::Get().Release(UIMeshes[0]->geometries[0]);
	UIMeshes[0]->geometries[0] = GeometrySystem::Get().AcquireFromConfig(UIConfig, true);
}

void LoadScene1(GameInstance* GameInst) {
	
}

void LoadScene2(GameInstance* GameInst) {
	for (size_t i = GameInst->Meshes.Size() - 1; i >= 3; --i) {
		AStaticMeshActor* M = GameInst->Meshes[i];
		DeleteObject(M);
		GameInst->Meshes[i] = nullptr;
		GameInst->Meshes.Pop();
	}

	AStaticMeshActor* Model1 = NewObject<AStaticMeshActor>("sponza");
	Model1->LoadFromResource("sponza");
	UTransformComponent* TansformComp1 = Model1->GetComponent<UTransformComponent>();
	TansformComp1->SetLocation(Vector(0.0f, -10.0f, 0.0f));
	TansformComp1->SetRotation(Vector(0.0f, 90.0f, 0.0f));
	TansformComp1->SetScale(Vector(0.1f));
	GameInst->Meshes.Push(Model1);

	AStaticMeshActor* Model2 = NewObject<AStaticMeshActor>("bunny");
	Model2->LoadFromResource("bunny");
	UTransformComponent* TansformComp2 = Model2->GetComponent<UTransformComponent>();
	TansformComp2->SetLocation(Vector(30.0f, 0.0f, 0.0f));
	TansformComp2->SetScale(Vector(5.0f));
	GameInst->Meshes.Push(Model2);

	AStaticMeshActor* Model3 = NewObject<AStaticMeshActor>("falcon");
	Model3->LoadFromResource("falcon");
	UTransformComponent* TansformComp3 = Model3->GetComponent<UTransformComponent>();
	TansformComp3->SetLocation(Vector(-30.0f, 0.0f, 0.0f));
	GameInst->Meshes.Push(Model3);
}

void LoadScene3(GameInstance* GameInst) {
	
}

void LoadScene4(GameInstance* GameInst) {
	
}