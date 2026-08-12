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
#include "Framework/Components/CameraComponent.h"
#include <Scene/World.h>
#include "Framework/Classes/SkyboxActor.h"

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

bool GameInstance::Boot() {
	if (!IGame::Boot()) {
		return false;
	}

	GLOG(Log::eInfo, "Booting...");

	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	JsonObject Content = JsonObject(MaterialAsset.ReadText());
	WindowSize.Width = (uint16_t)Content.ReadInt("Window.Width");
	WindowSize.Height = (uint16_t)Content.ReadInt("Window.Height");

	GameConsole = NewObject<DebugConsoleActor>("Debug console panel");

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

	WorldCamera = CameraSystem::Get().GetMainCamera();
	if (WorldCamera) {
		UCameraComponent* CameraComp = WorldCamera->GetCameraComponent();
		if (CameraComp) {
			CameraComp->SetPosition(Position);
			CameraComp->SetEulerAngles(Rotation);
		}
	}
	

	// Create test ui text objects.
	FString TestTextName = "Ubuntu Mono 21px";
	FString TestTextContent = "Test! \n Yooo!";
	TestText = NewObject<ATextActor>("Render information window.", UITextType::eUI_Text_Type_Bitmap, TestTextName, 21, TestTextContent);
	if (TestText) {
		TestText->SetActorLocation(Vector3(150, 450, 0));
		World->AddActor(TestText);
	}

	FString TestSystemName = "Noto Sans CJK JP";
	FString TestSystemContent = "Keyboard map:\
		\nLoad models:\
		\n\tO: Scene1 P: Scene2\
		\n\tK: Scene3 L: Scene4\
		\nM: Watch memory usage.\
		\nF1: Default view.\
		\nF2: Normal view.\
		\nF3: Material view.\
		\nF4: Depth view.";
	TestSysText = NewObject<ATextActor>("Keyboard map texts.", UITextType::eUI_Text_Type_System, TestSystemName, 25, TestSystemContent);

	if (TestSysText) {
		TestSysText->SetActorLocation(Vector3(100, 200, 0));
		World->AddActor(TestSysText);
	}

	// Load console
	GameConsole->Initialize();
	World->AddActor(GameConsole->GetText());
	World->AddActor(GameConsole->GetEntryText());

	// Skybox
	SB = NewObject<USkybox>();
	if (!SB) {
		GLOG(Log::eError, "Failed to create skybox. Exiting...");
		return false;
	}

	if (!SB->Create("SkyboxCube")) {
		GLOG(Log::eError, "Failed to create skybox. Exiting...");
		return false;
	}

	ASkyboxActor* SkyboxActor = NewObject<ASkyboxActor>("SkyboxActor");
	if (SkyboxActor) {
		World->AddActor(SkyboxActor);
	}

	// World meshes
	ARotationCubeActor* CubeMesh = NewObject<ARotationCubeActor>("TestCube");
	CubeMesh->SetActorLocation(Vector(0.0f, 0.0f, 0.0f));
	CubeMesh->SetActorRotation(Vector(0.0f));
	Meshes.Push(CubeMesh);
	World->AddActor(CubeMesh);

	ARotationCubeActor* CubeMesh2 = NewObject<ARotationCubeActor>("TestCube2");
	CubeMesh2->SetActorLocation(Vector(10.0f, 0.0f, 0.0f));
	CubeMesh2->SetActorRotation(Vector(0.0f));
	CubeMesh2->SetWorldScale(Vector(0.5f));
	CubeMesh2->AttachTo(CubeMesh);
	Meshes.Push(CubeMesh2);
	World->AddActor(CubeMesh2);

	ARotationCubeActor* CubeMesh3 = NewObject<ARotationCubeActor>("TestCube3");
	CubeMesh3->SetActorLocation(Vector(15.0f, 0.0f, 0.0f));
	CubeMesh3->SetActorRotation(Vector(0.0f));
	CubeMesh3->SetWorldScale(Vector(0.3f));
	CubeMesh3->AttachTo(CubeMesh2);
	Meshes.Push(CubeMesh3);
	World->AddActor(CubeMesh3);

	// TODO: TEMP
	EngineEvent::Register(eEventCode::Debug_0, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_1, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_2, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_3, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Object_Hover_ID_Changed, this, GameOnEvent);
	// TEMP

	return true;
}

void GameInstance::BeginPlay() {
	if (!World) return;
	World->BeginPlay();
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

	for (AStaticMeshActor* m : UIMeshes) {
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

	UCameraComponent* CameraComp = WorldCamera->GetCameraComponent();
	if (!CameraComp) {
		GLOG(Log::eError, "RenderViewSkybox::OnBuildPacke() Camera is nullptr.");
		return;
	}

	JsonObject Content = JsonObject(MaterialAsset);
	Content.WriteInt("Window.Width", (int)WindowSize.Width);
	Content.WriteInt("Window.Height", (int)WindowSize.Height);
	Content.WriteVector3("Camera.Position", CameraComp->GetPosition());
	Content.WriteVector3("Camera.Rotation", CameraComp->GetEulerAngles());
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
		AActor* Actor = Meshes[i];
		if (Actor->IsEnableTick()) {
			Actor->Tick(delta_time);
		}
	}

	for(int i = 0; i < UIMeshes.Size(); ++i) {
		if (UIMeshes[i]->IsEnableTick()) UIMeshes[i]->Tick(delta_time);
	}

	int px, py, cx, cy;
	Controller::GetMousePosition(cx, cy);
	Controller::GetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	UCameraComponent* CameraComp = WorldCamera->GetCameraComponent();
	if (!CameraComp) {
		return false;
	}

	if (Controller::IsButtonDown(eButtons::Right)) {
		if (cx != px) {
			CameraComp->RotateYaw((px - cx) * MouseMoveSpeed);
		}

		if (cy != py) {
			CameraComp->RotatePitch((py - cy) * MouseMoveSpeed);
		}
	}

	// Text
	WorldCamera = CameraSystem::Get().GetMainCamera();
	Vector3 Pos = CameraComp->GetPosition();
	Vector3 Rot = CameraComp->GetEulerAngles();

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
	Vector3 Forward = CameraComp->Forward();
	Vector3 Right = CameraComp->Right();
	Vector3 Up = CameraComp->Up();

	// NOTE: starting at a reasonable default to avoid too many realloc.
	uint32_t DrawCount = (uint32_t)World->GetVisibleGeometryCount();

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
	TestText->SetText(FPSText);

	GameConsole->Tick(delta_time);

	if (World) {
		World->Tick(delta_time);
	}

	return true;
}

static float GameTime = 0.0f;

bool GameInstance::Render(SRenderPacket* packet, float delta_time) {
	GameTime += delta_time;

	// TODO: Read from config.
	packet->view_count = 0;
	packet->views.resize(packet->view_count);
	uint32_t ViewCounter = 0;

	//IRenderView* PickView = RenderviewSys.Get(ERenderViewType::Pick);
	//if (PickView) {
	//	// Pick uses both world and ui packet data.
	//	PickPacketData PickPacket;
	//	PickPacket.UIMeshData = UIPacket.meshData;
	//	PickPacket.WorldMeshData = std::vector<GeometryRenderData>();
	//	PickPacket.Texts = UIPacket.Textes;
	//	PickPacket.TextCount = UIPacket.textCount;

	//	if (!RenderviewSys.BuildPacket(PickView, &PickPacket, &packet->views[ViewCounter++])) {
	//		GLOG(Log::eError, "Failed to build packet for view 'Pick'.");
	//		return false;
	//	}
	//}

	return true;
}

void GameInstance::OnResize(unsigned int width, unsigned int height) {
	WindowSize = { (uint16_t)width, (uint16_t)height };

	TestText->SetActorLocation(Vector3(180, (float)height - 150, 0));
	TestSysText->SetActorLocation(Vector3(100, (float)height - 400, 0));
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
	Model1->SetActorLocation(Vector(0.0f, -10.0f, 0.0f));
	Model1->SetActorRotation(Vector(0.0f, 90.0f, 0.0f));
	Model1->SetWorldScale(Vector(0.1f));
	GameInst->GetWorld()->AddActor(Model1);

	AStaticMeshActor* Model2 = NewObject<AStaticMeshActor>("bunny");
	Model2->LoadFromResource("bunny");
	Model2->SetActorLocation(Vector(30.0f, 0.0f, 0.0f));
	Model2->SetActorRotation(Vector(0.0f, 90.0f, 0.0f));
	Model2->SetWorldScale(Vector(5.0f));
	GameInst->GetWorld()->AddActor(Model2);

	AStaticMeshActor* Model3 = NewObject<AStaticMeshActor>("falcon");
	Model3->LoadFromResource("falcon");
	Model3->SetActorLocation(Vector(-30.0f, 0.0f, 0.0f));
	GameInst->GetWorld()->AddActor(Model3);
}

void LoadScene3(GameInstance* GameInst) {
	
}

void LoadScene4(GameInstance* GameInst) {
	
}