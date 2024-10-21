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

static bool EnableFrustumCulling = false;

bool ConfigureRenderviews(SApplicationConfig* config);

bool GameOnEvent(unsigned short code, void* sender, void* listender_inst, SEventContext context) {
	GameInstance* GameInst = (GameInstance*)listender_inst;

	switch (code)
	{
	case Core::eEvent_Code_Object_Hover_ID_Changed: 
	{
		GameInst->HoveredObjectID = context.data.u32[0];
		return true;
	}break;
	case Core::eEvent_Code_Reload_Shader_Module:
	{
		for (uint32_t i = 0; i < 10; ++i) {
			if (GameInst->Meshes[i].Generation != INVALID_ID_U8) {
				GameInst->Meshes[i].ReloadMaterial();
			}
		}
	}
	}

	return false;
}

bool GameOnDebugEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	GameInstance* GameInst = (GameInstance*)listener_instance;

	if (code == Core::eEvent_Code_Debug_0) {
		if (GameInst->SponzaMesh->Generation == INVALID_ID_U8) {
			LOG_DEBUG("Loading sponza...");

			if (!GameInst->SponzaMesh->LoadFromResource("sponza")) {
				LOG_ERROR("Failed to load sponza mesh!");
			}
		}

		return true;
	}
	else if (code == Core::eEvent_Code_Debug_1) {
		if (GameInst->CarMesh->Generation == INVALID_ID_U8) {
			LOG_DEBUG("Loading falcon...");

			if (!GameInst->CarMesh->LoadFromResource("falcon")) {
				LOG_ERROR("Failed to load falcon mesh!");
			}
		}

		return true;
	}
	else if (code == Core::eEvent_Code_Debug_2) {
		if (GameInst->BunnyMesh->Generation == INVALID_ID_U8) {
			LOG_DEBUG("Loading bunny...");

			if (!GameInst->BunnyMesh->LoadFromResource("bunny")) {
				LOG_ERROR("Failed to load falcon mesh!");
			}
		}

		return true;
	}
	else if (code == Core::eEvent_Code_Debug_3) {
		if (GameInst->DragonMesh->Generation == INVALID_ID_U8) {
			LOG_DEBUG("Loading dragon...");

			if (!GameInst->DragonMesh->LoadFromResource("dragon")) {
				LOG_ERROR("Failed to load falcon mesh!");
			}
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
		unsigned short KeyCode = context.data.u16[0];

		return true;
	}

	return false;
}

bool GameInstance::Boot(IRenderer* renderer) {
	LOG_INFO("Booting...");

	Renderer = renderer;
	SApplicationConfig* Config = &app_config;
	
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

bool GameInstance::Initialize() {
	LOG_DEBUG("GameInitialize() called.");

	// Load python script
	TestPython.SetPythonFile("recompile_shader");

	WorldCamera = CameraSystem::GetDefault();
	WorldCamera->SetPosition(Vec3(0.0f, 0.0f, 40.0f));

	// Create test ui text objects.
	if (!TestText.Create(Renderer, UITextType::eUI_Text_Type_Bitmap, "Ubuntu Mono 21px", 21, "Test! \n Yooo!")) {
		LOG_ERROR("Failed to load basic ui bitmap text.");
		return false;
	}
	TestText.SetPosition(Vec3(150, 450, 0));

	if (!TestSysText.Create(Renderer, UITextType::eUI_Text_Type_system, 
		"Noto Sans CJK JP", 26, "Keyboard map:\
		\nLoad models:\
		\n\tO: sponza P: car\
		\n\tK: dragon L: bunny\
		\nM: Watch memory usage.\
		\nF1: Default shader mode.\
		\nF2: Lighting shader mode.\
		\nF3: Normals shader mode."))
	{
		LOG_ERROR("Failed to load basic ui system text.");
		return false;
	}
	TestSysText.SetPosition(Vec3(100, 200, 0));

	// Skybox
	if (!SB.Create("SkyboxCube", Renderer)) {
		LOG_ERROR("Failed to create skybox. Exiting...");
		return false;
	}

	// World meshes
	Meshes.resize(10);
	UIMeshes.resize(10);
	for (uint32_t i = 0; i < 10; ++i) {
		Meshes[i].Generation = INVALID_ID_U8;
		UIMeshes[i].Generation = INVALID_ID_U8;
	}

	Mesh* CubeMesh = &Meshes[0];
	CubeMesh->geometry_count = 1;
	CubeMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.World");
	CubeMesh->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig, true);
	CubeMesh->Generation = 0;
	CubeMesh->UniqueID = Identifier::AcquireNewID(CubeMesh);
	CubeMesh->Transform = Transform();

	Mesh* CubeMesh2 = &Meshes[1];
	CubeMesh2->geometry_count = 1;
	CubeMesh2->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh2->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig2 = GeometrySystem::GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "TestCube2", "Material.World");
	CubeMesh2->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig2, true);
	CubeMesh2->Transform = Transform(Vec3(10.0f, 0.0f, 1.0f));
	CubeMesh2->Generation = 0;
	CubeMesh2->UniqueID = Identifier::AcquireNewID(CubeMesh2);
	CubeMesh2->Transform.SetParentTransform(&CubeMesh->Transform);

	Mesh* CubeMesh3 = &Meshes[2];
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

	CarMesh = &Meshes[3];
	CarMesh->Transform = Transform(Vec3(15.0f, 0.0f, -15.0f));
	CarMesh->UniqueID = Identifier::AcquireNewID(CarMesh);

	SponzaMesh = &Meshes[4];
	SponzaMesh->Transform = Transform(Vec3(0.0f, -10.0f, 0.0f), Quaternion(Vec3(0.0f, 90.0f, 0.0f)), Vec3(0.1f));
	SponzaMesh->UniqueID = Identifier::AcquireNewID(SponzaMesh);

	BunnyMesh = &Meshes[5];
	BunnyMesh->Transform = Transform(Vec3(30.0f, 0.0f, -30.0f), Quaternion(Vec3(0.0f, 0.0f, 0.0f)), Vec3(5.0f));
	BunnyMesh->UniqueID = Identifier::AcquireNewID(BunnyMesh);

	DragonMesh = &Meshes[6];
	DragonMesh->Transform = Transform(Vec3(45.0f, 0.0f, -45.0f), Quaternion(Vec3(0.0f, 0.0f, 0.0f)), Vec3(1.0f));
	DragonMesh->UniqueID = Identifier::AcquireNewID(DragonMesh);

	// Load up some test UI geometry.
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

	const float h = app_config.start_height / 3.0f;
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
	Mesh* UIMesh = &UIMeshes[0];
	UIMesh->geometry_count = 1;
	UIMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*), MemoryType::eMemory_Type_Array);
	UIMesh->geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
	UIMesh->Generation = 0;
	UIMesh->UniqueID = Identifier::AcquireNewID(UIMesh);
	UIMesh->Transform = Transform();

	// TODO: TEMP
	Core::EventRegister(Core::eEvent_Code_Debug_0, this, GameOnDebugEvent);
	Core::EventRegister(Core::eEvent_Code_Debug_1, this, GameOnDebugEvent);
	Core::EventRegister(Core::eEvent_Code_Debug_2, this, GameOnDebugEvent);
	Core::EventRegister(Core::eEvent_Code_Debug_3, this, GameOnDebugEvent);
	Core::EventRegister(Core::eEvent_Code_Object_Hover_ID_Changed, this, GameOnEvent);
	Core::EventRegister(Core::eEvent_Code_Reload_Shader_Module, this, GameOnEvent);
	// TEMP

	Core::EventRegister(Core::eEvent_Code_Key_Pressed, this, GameOnKey);
	Core::EventRegister(Core::eEvent_Code_Key_Released, this, GameOnKey);

	return true;
}

void GameInstance::Shutdown() {
	// TODO: Temp
	SB.Destroy();

	TestText.Destroy();
	TestSysText.Destroy();

	// TODO: TEMP
	Core::EventUnregister(Core::eEvent_Code_Debug_0, this, GameOnDebugEvent);
	Core::EventUnregister(Core::eEvent_Code_Debug_1, this, GameOnDebugEvent);
	Core::EventUnregister(Core::eEvent_Code_Debug_2, this, GameOnDebugEvent);
	Core::EventUnregister(Core::eEvent_Code_Debug_3, this, GameOnDebugEvent);
	Core::EventUnregister(Core::eEvent_Code_Object_Hover_ID_Changed, this, GameOnEvent);
	Core::EventUnregister(Core::eEvent_Code_Reload_Shader_Module, this, GameOnEvent);
	// TEMP

	Core::EventUnregister(Core::eEvent_Code_Key_Pressed, this, GameOnKey);
	Core::EventUnregister(Core::eEvent_Code_Key_Released, this, GameOnKey);
}

bool GameInstance::Update(float delta_time) {
	// Ensure this is cleaned up to avoid leaking memory.
	// TODO: Need a version of this that uses the frame allocator.
	if (!FrameData.WorldGeometries.empty()) {
		FrameData.WorldGeometries.clear();
		std::vector<GeometryRenderData>().swap(FrameData.WorldGeometries);
	}

	static size_t AllocCount = 0;
	size_t PrevAllocCount = AllocCount;
	AllocCount = Memory::GetAllocateCount();
	if (Core::InputIsKeyUp(eKeys_M) && Core::InputWasKeyDown(eKeys_M)) {
		char* Usage = Memory::GetMemoryUsageStr();
		LOG_INFO(Usage);
		StringFree(Usage);
		LOG_DEBUG("Allocations: %llu (%llu this frame)", AllocCount, AllocCount - PrevAllocCount);
	}

	// Temp shader debug
	if (Core::InputIsKeyUp(eKeys_F1) && Core::InputWasKeyDown(eKeys_F1)) {
		SEventContext Context;
		Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Default;
		Core::EventFire(Core::eEvent_Code_Set_Render_Mode, nullptr, Context);
	}
	if (Core::InputIsKeyUp(eKeys_F2) && Core::InputWasKeyDown(eKeys_F2)) {
		SEventContext Context;
		Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Lighting;
		Core::EventFire(Core::eEvent_Code_Set_Render_Mode, nullptr, Context);
	}
	if (Core::InputIsKeyUp(eKeys_F3) && Core::InputWasKeyDown(eKeys_F3)) {
		SEventContext Context;
		Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Normals;
		Core::EventFire(Core::eEvent_Code_Set_Render_Mode, nullptr, Context);
	}

	if (Core::InputIsKeyDown(eKeys_Left)) {
		WorldCamera->RotateYaw(1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(eKeys_Right)) {
		WorldCamera->RotateYaw(-1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Up)) {
		WorldCamera->RotatePitch(1.0f * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Down)) {
		WorldCamera->RotatePitch(-1.0f * delta_time);
	}

	float TempMoveSpeed = 50.0f;
	if (Core::InputIsKeyDown(Keys::eKeys_Shift)) {
		TempMoveSpeed *= 2.0f;
	}

	if (Core::InputIsKeyDown(Keys::eKeys_W)) {
		WorldCamera->MoveForward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_S)) {
		WorldCamera->MoveBackward(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_A)) {
		WorldCamera->MoveLeft(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_D)) {
		WorldCamera->MoveRight(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_Q)) {
		WorldCamera->MoveDown(TempMoveSpeed * delta_time);
	}
	if (Core::InputIsKeyDown(Keys::eKeys_E)) {
		WorldCamera->MoveUp(TempMoveSpeed * delta_time);
	}

	if (Core::InputIsKeyDown(Keys::eKeys_R)) {
		WorldCamera->Reset();
	}

	// TODO: Remove
	if (Core::InputIsKeyUp(eKeys_O) && Core::InputWasKeyDown(eKeys_O)) {
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Debug_0, this, Context);
	}

	if (Core::InputIsKeyUp(eKeys_L) && Core::InputWasKeyDown(eKeys_L)) {
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Debug_2, this, Context);
	}

	if (Core::InputIsKeyUp(eKeys_K) && Core::InputWasKeyDown(eKeys_K)) {
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Debug_3, this, Context);
	}

	if (Core::InputIsKeyUp(eKeys_P) && Core::InputWasKeyDown(eKeys_P)) {
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Debug_1, this, Context);
	}

	if (Core::InputIsKeyUp(eKeys_G) && Core::InputWasKeyDown(eKeys_G)) {
		TestPython.ExecuteFunc("CompileShaders", "glsl");

		// Reload
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Reload_Shader_Module, this, Context);
	}
	if (Core::InputIsKeyUp(eKeys_H) && Core::InputWasKeyDown(eKeys_H)) {
		TestPython.ExecuteFunc("CompileShaders", "hlsl");

		// Reload
		SEventContext Context = {};
		Core::EventFire(Core::eEvent_Code_Reload_Shader_Module, this, Context);
	}
	// Remove

	int px, py, cx, cy;
	Core::InputGetMousePosition(cx, cy);
	Core::InputGetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	if (Core::InputeIsButtonDown(eButton_Right)) {
		if (cx != px) {
			WorldCamera->RotateYaw((px - cx) * MouseMoveSpeed);
		}
		if (cy != py) {
			WorldCamera->RotatePitch((py - cy) * MouseMoveSpeed);
		}
	}

	Quaternion Rotation = QuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f * (float)delta_time, false);
	Meshes[0].Transform.Rotate(Rotation);
	Meshes[1].Transform.Rotate(Rotation);
	Meshes[2].Transform.Rotate(Rotation);

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
	float MouseX_NDC = RangeConvertfloat((float)MouseX, 0.0f, (float)Width, -1.0f, 1.0f);
	float MouseY_NDC = RangeConvertfloat((float)MouseY, 0.0f, (float)Height, -1.0f, 1.0f);

	double FPS, FrameTime;
	Metrics::Frame(&FPS, &FrameTime);

	// Update the frustum.
	Vec3 Forward = WorldCamera->Forward();
	Vec3 Right = WorldCamera->Right();
	Vec3 Up = WorldCamera->Up();
	// TODO: Get camera fov, aspect etc.
	CameraFrustum = Frustum(WorldCamera->GetPosition(), Forward, Right, Up, (float)Width / (float)Height, Deg2Rad(45.0f), 0.1f, 1000.0f);

	// NOTE: starting at a reasonable default to avoid too many realloc.
	uint32_t DrawCount = 0;
	FrameData.WorldGeometries.reserve(512);
	for (uint32_t i = 0; i < 10; ++i) {
		Mesh* m = &Meshes[i];
		if (m == nullptr) {
			continue;
		}

		if (m->Generation != INVALID_ID_U8) {
			Matrix4 Model = m->Transform.GetWorldTransform();

			for (uint32_t j = 0; j < m->geometry_count; j++) {
				Geometry* g = m->geometries[j];
				if (g == nullptr) {
					continue;
				}

				// Bounding sphere calculation
				//{
				//	Vec3 ExtensMin = g->Extents.min.Transform(Model);
				//	Vec3 ExtensMax = g->Extents.max.Transform(Model);

				//	float Min = DMIN(DMIN(ExtensMin.x, ExtensMin.y), ExtensMin.z);
				//	float Max = DMIN(DMIN(ExtensMax.x, ExtensMax.y), ExtensMax.z);
				//	float Diff = Dabs(Max - Min);
				//	float Radius = Diff / 2.0f;

				//	// Translate/scale the center.
				//	Vec3 Center = g->Center.Transform(Model);

				//	if (State->CameraFrustum.IntersectsSphere(Center, Radius)) {
				//		// Add it to the list to be rendered.
				//		GeometryRenderData Data;
				//		Data.model = Model;
				//		Data.geometry = g;
				//		Data.uniqueID = m->UniqueID;
				//		game_instance->FrameData.WorldGeometries.push_back(Data);
				//		DrawCount++;
				//	}
				//}

				// AABB calculation
				{
					// Translate/scale the extents.
					Vec3 ExtentsMax = g->Extents.max.Transform(Model);

					// Translate/scale the center.
					Vec3 Center = g->Center.Transform(Model);
					Vec3 HalfExtents = {                                     
						Dabs(ExtentsMax.x - Center.x),
						Dabs(ExtentsMax.y - Center.y),
						Dabs(ExtentsMax.z - Center.z)
					};

					if (CameraFrustum.IntersectsAABB(Center, HalfExtents) && EnableFrustumCulling) {
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model = Model;
						Data.geometry = g;
						Data.uniqueID = m->UniqueID;
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
					else if (!EnableFrustumCulling){
						// Add it to the list to be rendered.
						GeometryRenderData Data;
						Data.model = Model;
						Data.geometry = g;
						Data.uniqueID = m->UniqueID;
						FrameData.WorldGeometries.push_back(Data);
						DrawCount++;
					}
				}
			}
		}
	}


	// TODO: Temp
	char FPSText[512];
	StringFormat(FPSText, 512,
		"\
	Camera Pos: [%.3f %.3f %.3f]\tCamera Rot: [%.3f %.3f %.3f]\n\
	L=%s R=%s\tNDC: x=%.2f, y=%.2f\tHovered: %s%u\n\
	FPS: %d\tDelta time: %.2f\n\
	Drawn Count: %-5u",
		Pos.x, Pos.y, Pos.z,
		Rad2Deg(Rot.x), Rad2Deg(Rot.y), Rad2Deg(Rot.z),
		LeftDown ? "Y" : "N", RightDown ? "Y" : "N",
		MouseX_NDC, MouseY_NDC,
		HoveredObjectID == INVALID_ID ? "None" : "",
		HoveredObjectID == INVALID_ID ? 0 : HoveredObjectID,
		(int)FPS,
		(float)FrameTime,
		DrawCount
	);
	TestText.SetText(FPSText);

	return true;
}

bool GameInstance::Render(SRenderPacket* packet, float delta_time) {

	// TODO: Read from config.
	packet->view_count = 4;
	std::vector<RenderViewPacket> Views;
	Views.resize(packet->view_count);
	packet->views = Views;

	// Skybox
	SkyboxPacketData SkyboxData;
	SkyboxData.sb = &SB;
	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("Skybox"), &SkyboxData, &packet->views[0])) {
		LOG_ERROR("Failed to build packet for view 'World_Opaque'.");
		return false;
	}

	// World
	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("World"), &FrameData.WorldGeometries, &packet->views[1])) {
		LOG_ERROR("Failed to build packet for view 'World'.");
		return false;
	}

	// UI
	uint32_t UIMeshCount = 0;
	Mesh** TempUIMeshes = (Mesh**)Memory::Allocate(sizeof(Mesh*) * 10, MemoryType::eMemory_Type_Array);
	// TODO: Flexible size array.
	for (uint32_t i = 0; i < 10; ++i) {
		if (UIMeshes[i].Generation != INVALID_ID_U8) {
			TempUIMeshes[UIMeshCount] = &UIMeshes[i];
			UIMeshCount++;
		}
	}


	UIText** Texts = (UIText**)Memory::Allocate(sizeof(UIText*) * 2, MemoryType::eMemory_Type_Array);
	Texts[0] = &TestText;
	Texts[1] = &TestSysText;

	UIPacketData UIPacket;
	UIPacket.meshData.mesh_count = UIMeshCount;
	UIPacket.meshData.meshes = TempUIMeshes;
	UIPacket.textCount = 2;
	UIPacket.Textes = Texts;

	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("UI"), &UIPacket, &packet->views[2])) {
		LOG_ERROR("Failed to build packet for view 'UI'.");
		return false;
	}

	// Pick uses both world and ui packet data.
	PickPacketData PickPacket;
	PickPacket.UIMeshData = UIPacket.meshData;
	PickPacket.WorldMeshData = FrameData.WorldGeometries;
	PickPacket.Texts = UIPacket.Textes;
	PickPacket.TextCount = UIPacket.textCount;

	if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("Pick"), &PickPacket, &packet->views[3])) {
		LOG_ERROR("Failed to build packet for view 'Pick'.");
		return false;
	}

	return true;
}

void GameInstance::OnResize(unsigned int width, unsigned int height) {
	Width = width;
	Height = height;

	TestText.SetPosition(Vec3(180, (float)height - 150, 0));
	TestSysText.SetPosition(Vec3(100, (float)height - 400, 0));

	// TODO: Temp
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

	const float h = Height / 3.0f;
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

	UIMeshes[0].geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
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
