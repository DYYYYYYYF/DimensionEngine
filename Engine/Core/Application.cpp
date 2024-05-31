#include "Application.hpp"

#include "EngineLogger.hpp"
#include "Event.hpp"
#include "Input.hpp"
#include "DMemory.hpp"
#include "Clock.hpp"

#include "GameType.hpp"
#include "Platform/Platform.hpp"

#include "Renderer/RendererFrontend.hpp"
#include "Math/MathTypes.hpp"

// Systems
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderViewSystem.hpp"

#include "Math/GeometryUtils.hpp"
#include "Resources/Mesh.hpp"


struct SApplicationState {
	SGame* game_instance = nullptr;
	SPlatformState platform;

	bool is_running;
	bool is_suspended;
	short width;
	short height;
	double last_time;
	SClock clock;

	// Temp
	Skybox SB;
	std::vector<Mesh> Meshes;
	std::vector<Mesh> UIMeshes;
};

static bool Initialized = false;
static SApplicationState AppState;
static IRenderer* Renderer = nullptr;
static TextureSystem TextureManager;

bool ApplicationCreate(SGame* game_instance){
	if (Initialized) {
		UL_ERROR("Create application more than once!");
		return false;
	}

	AppState.game_instance = game_instance;

	// Init logger
	static EngineLogger* GlobalLogger  = new EngineLogger();
	Core::InputInitialize();

	LOG_INFO("Test Info");
	LOG_DEBUG("Test Debug");
	LOG_ERROR("Test Error");
	LOG_WARN("Test Warn");
	LOG_FATAL("Test Fatal");

	AppState.is_running = true;
	AppState.is_suspended = false;

	if (!Core::EventInitialize()) {
		UL_ERROR("Event system init failed. Application can not start.");
		return false;
	}

	Core::EventRegister(Core::eEvent_Code_Application_Quit, 0, ApplicationOnEvent);
	Core::EventRegister(Core::eEvent_Code_Key_Pressed, 0, ApplicationOnKey);
	Core::EventRegister(Core::eEvent_Code_Key_Released, 0, ApplicationOnKey);
	Core::EventRegister(Core::eEvent_Code_Resize, 0, ApplicationOnResized);

	Platform::PlatformStartup(&AppState.platform,
		game_instance->app_config.name, 
		game_instance->app_config.start_x, 
		game_instance->app_config.start_y, 
		game_instance->app_config.start_width, 
		game_instance->app_config.start_height);

	AppState.width = game_instance->app_config.start_width;
	AppState.height = game_instance->app_config.start_height;

	// Init texture system
	SResourceSystemConfig ResourceSystemConfig;
	ResourceSystemConfig.max_loader_count = 32;
	ResourceSystemConfig.asset_base_path = "../Assets";
	if (!ResourceSystem::Initialize(ResourceSystemConfig)) {
		UL_FATAL("Resource system failed to initialize!");
		return false;
	}

	// Init Renderer
	if (Renderer == nullptr) {
		void* TempRenderer = (IRenderer*)Memory::Allocate(sizeof(IRenderer), MemoryType::eMemory_Type_Renderer);
		Renderer = new(TempRenderer)IRenderer(eRenderer_Backend_Type_Vulkan, &AppState.platform);
		ASSERT(Renderer);
	}

	// Init shader system
	SShaderSystemConfig ShaderSystemConfig;
	ShaderSystemConfig.max_shader_count = 1024;
	ShaderSystemConfig.max_uniform_count = 128;
	ShaderSystemConfig.max_global_textures = 31;
	ShaderSystemConfig.max_instance_textures = 31;
	if (!ShaderSystem::Initialize(Renderer, ShaderSystemConfig)) {
		UL_FATAL("Shader system failed to initialize!");
		return false;
	}

	if (!Renderer->Initialize(game_instance->app_config.name, &AppState.platform)) {
		UL_FATAL("Renderer failed to initialize!");
		return false;
	}

	// Init texture system
	STextureSystemConfig TextureSystemConfig;
	TextureSystemConfig.max_texture_count = 65536;
	if (!TextureSystem::Initialize(Renderer, TextureSystemConfig)) {
		UL_FATAL("Texture system failed to initialize!");
		return false;
	}

	// Init material system
	SMaterialSystemConfig MaterialSystemConfig;
	MaterialSystemConfig.max_material_count = 4096;
	if (!MaterialSystem::Initialize(Renderer, MaterialSystemConfig)) {
		UL_FATAL("Material system failed to initialize!");
		return false;
	}

	// Init geometry system
	SGeometrySystemConfig GeometrySystemConfig;
	GeometrySystemConfig.max_geometry_count = 4096;
	if (!GeometrySystem::Initialize(Renderer, GeometrySystemConfig)) {
		UL_FATAL("Geometry system failed to initialize!");
		return false;
	}

	// Init camera system
	SCameraSystemConfig CameraSystemConfig;
	CameraSystemConfig.max_camera_count = 61;
	if (!CameraSystem::Initialize(Renderer, CameraSystemConfig)) {
		UL_FATAL("Camera system failed to initialize!");
		return false;
	}

	SRenderViewSystemConfig RenderViewSysConfig;
	RenderViewSysConfig.max_view_count = 255;
	if (!RenderViewSystem::Initialize(Renderer, RenderViewSysConfig)) {
		UL_FATAL("Render view system failed to intialize!");
		return false;
	}

	// Load render views.
	RenderViewConfig SkyboxConfig;
	SkyboxConfig.type = RenderViewKnownType::eRender_View_Known_Type_Skybox;
	SkyboxConfig.width = 0;
	SkyboxConfig.height = 0;
	SkyboxConfig.name = "Skybox";
	SkyboxConfig.pass_count = 1;
	std::vector<RenderViewPassConfig> SkyboxPasses(1);
	SkyboxPasses[0].name = "Renderpass.Builtin.Skybox";
	SkyboxConfig.passes = SkyboxPasses;
	SkyboxConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	if (!RenderViewSystem::Create(SkyboxConfig)) {
		UL_FATAL("Failed to create skybox view. Aborting application.");
		return false;
	}

	RenderViewConfig OpaqueWorldConfig;
	OpaqueWorldConfig.type = RenderViewKnownType::eRender_View_Known_Type_World;
	OpaqueWorldConfig.width = 0;
	OpaqueWorldConfig.height = 0;
	OpaqueWorldConfig.name = "World_Opaque";
	OpaqueWorldConfig.pass_count = 1;
	std::vector<RenderViewPassConfig> Passes(1);
	Passes[0].name = "Renderpass.Builtin.World";
	OpaqueWorldConfig.passes = Passes;
	OpaqueWorldConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	if (!RenderViewSystem::Create(OpaqueWorldConfig)) {
		UL_FATAL("Failed to create world view. Aborting application.");
		return false;
	}

	RenderViewConfig UIViewConfig;
	UIViewConfig.type = RenderViewKnownType::eRender_View_Known_Type_UI;
	UIViewConfig.width = 0;
	UIViewConfig.height = 0;
	UIViewConfig.name = "UI";
	UIViewConfig.pass_count = 1;
	std::vector<RenderViewPassConfig> UIPasses(1);
	UIPasses[0].name = "Renderpass.Builtin.UI";
	UIViewConfig.passes = UIPasses;
	UIViewConfig.view_matrix_source = RenderViewViewMatrixtSource::eRender_View_View_Matrix_Source_Scene_Camera;
	if (!RenderViewSystem::Create(UIViewConfig)) {
		UL_FATAL("Failed to create ui view. Aborting application.");
		return false;
	}

	// TODO: Temp
	// Skybox
	TextureMap* CubeMap = &AppState.SB.Cubemap;
	CubeMap->filter_magnify = TextureFilter::eTexture_Filter_Mode_Linear;
	CubeMap->filter_minify = TextureFilter::eTexture_Filter_Mode_Linear;
	CubeMap->usage = TextureUsage::eTexture_Usage_Map_Cubemap;
	if (!Renderer->AcquireTextureMap(CubeMap)) {
		UL_FATAL("Unable to acquire resources for cube map texture.");
		return false;
	}
	CubeMap->texture = TextureSystem::AcquireCube("skybox", true);
	SGeometryConfig SkyboxCubeConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "SkyboxCube", nullptr);
	// Clear out the material name.
	SkyboxCubeConfig.material_name[0] = '\0';
	AppState.SB.g = GeometrySystem::AcquireFromConfig(SkyboxCubeConfig, true);
	AppState.SB.RenderFrameNumber = INVALID_ID_U64;
	Shader* SkyboxShader = ShaderSystem::Get(BUILTIN_SHADER_NAME_SKYBOX);
	std::vector<TextureMap*> Maps = { &AppState.SB.Cubemap };

	AppState.SB.InstanceID = Renderer->AcquireInstanceResource(SkyboxShader, Maps);
	if (AppState.SB.InstanceID == INVALID_ID) {
		UL_FATAL("Unable to acquire shader resource for skybox texture.");
		return false;
	}

	// World meshes
	AppState.Meshes.resize(10);
	Mesh* CubeMesh = &AppState.Meshes[0];
	CubeMesh->geometry_count = 1;
	CubeMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "TestCube", "Material.World");
	CubeMesh->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig, true);
	CubeMesh->Transform = Transform();

	Mesh* CubeMesh2 = &AppState.Meshes[1];
	CubeMesh2->geometry_count = 1;
	CubeMesh2->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh2->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig2 = GeometrySystem::GenerateCubeConfig(5.0f, 5.0f, 5.0f, 1.0f, 1.0f, "TestCube2", "Material.World");
	CubeMesh2->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig2, true);
	CubeMesh2->Transform = Transform(Vec3(10.0f, 0.0f, 1.0f));
	CubeMesh2->Transform.SetParentTransform(&CubeMesh->Transform);

	Mesh* CubeMesh3 = &AppState.Meshes[2];
	CubeMesh3->geometry_count = 1;
	CubeMesh3->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CubeMesh3->geometry_count, MemoryType::eMemory_Type_Array);
	SGeometryConfig GeoConfig3 = GeometrySystem::GenerateCubeConfig(2.0f, 2.0f, 2.0f, 1.0f, 1.0f, "TestCube3", "Material.World");
	CubeMesh3->geometries[0] = GeometrySystem::AcquireFromConfig(GeoConfig3, true);
	CubeMesh3->Transform = Transform(Vec3(5.0f, 0.0f, 1.0f));
	CubeMesh3->Transform.SetParentTransform(&CubeMesh2->Transform);

	// Test mesh loaded from file.
	Mesh* CarMesh = &AppState.Meshes[3];
	Resource CarMeshResource;
	// Test model sponza/falcon
	if (!ResourceSystem::Load("falcon", ResourceType::eResource_type_Static_Mesh, nullptr, &CarMeshResource)) {
		UL_ERROR("Failed to load car test mesh.");
	}
	else {
		SGeometryConfig* Configs = (SGeometryConfig*)CarMeshResource.Data;
		CarMesh->geometry_count = (unsigned short)CarMeshResource.DataCount;
		CarMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * CarMesh->geometry_count, MemoryType::eMemory_Type_Array);
		for (uint32_t i = 0; i < CarMesh->geometry_count; ++i) {
			CarMesh->geometries[i] = GeometrySystem::AcquireFromConfig(Configs[i], true);
		}

		CarMesh->Transform = Transform(Vec3(15.0f, 0.0f, 0.0f), Quaternion::Identity(), Vec3(1.0f, 1.0f, 1.0f));
		ResourceSystem::Unload(&CarMeshResource);
	}

	Mesh* SponzaMesh = &AppState.Meshes[4];
	Resource SponzaMeshResource;
	// Test model sponza/falcon
	if (!ResourceSystem::Load("sponza", ResourceType::eResource_type_Static_Mesh, nullptr, &SponzaMeshResource)) {
		UL_ERROR("Failed to load car test mesh.");
	}
	else {
		SGeometryConfig* Configs = (SGeometryConfig*)SponzaMeshResource.Data;
		SponzaMesh->geometry_count = (unsigned short)SponzaMeshResource.DataCount;
		SponzaMesh->geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*) * SponzaMesh->geometry_count, MemoryType::eMemory_Type_Array);
		for (uint32_t i = 0; i < SponzaMesh->geometry_count; ++i) {
			SponzaMesh->geometries[i] = GeometrySystem::AcquireFromConfig(Configs[i], true);
		}

		SponzaMesh->Transform = Transform(Vec3(0.0f, -60.0f, 0.0f), Quaternion(Vec3(0.0f, 90.0f, 0.0f)), Vec3(0.1f, 0.1f, 0.1f));
		ResourceSystem::Unload(&SponzaMeshResource);
	}

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
	strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

	const float h = 256.0f;
	const float w = 64.0f;
	const float x = 0.0f;
	const float y = game_instance->app_config.start_height;

	Vertex2D UIVerts[4];
	UIVerts[0].position.x = x;
	UIVerts[0].position.y = y;
	UIVerts[0].texcoord.x = 0.0f;
	UIVerts[0].texcoord.y = 0.0f;

	UIVerts[1].position.x = x + h;
	UIVerts[1].position.y = y - w;
	UIVerts[1].texcoord.x = 1.0f;
	UIVerts[1].texcoord.y = 1.0f;

	UIVerts[2].position.x = x;
	UIVerts[2].position.y = y - w;
	UIVerts[2].texcoord.x = 0.0f;
	UIVerts[2].texcoord.y = 1.0f;

	UIVerts[3].position.x = x + h;
	UIVerts[3].position.y = y;
	UIVerts[3].texcoord.x = 1.0f;
	UIVerts[3].texcoord.y = 0.0f;

	UIConfig.vertices = UIVerts;

	// Indices
	uint32_t UIIndices[6] = { 0, 2, 1, 0, 1, 3 };
	UIConfig.indices = UIIndices;

	// Get UI geometry from config.
	Mesh UIMesh;
	UIMesh.geometry_count = 1;
	UIMesh.geometries = (Geometry**)Memory::Allocate(sizeof(Geometry*), MemoryType::eMemory_Type_Array);
	UIMesh.geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
	UIMesh.Transform = Transform();
	AppState.UIMeshes.push_back(UIMesh);

	// Init Game
	if (!AppState.game_instance->initialize(AppState.game_instance)) {
		UL_FATAL("Game failed to initialize!");
		return false;
	}

	AppState.game_instance->on_resize(AppState.game_instance, AppState.width, AppState.height);
	Renderer->OnResize(AppState.width, AppState.height);

	Initialized = true;
	return true;
}

bool ApplicationRun() {
	Clock::Start(&AppState.clock);
	Clock::Update(&AppState.clock);
	AppState.last_time = AppState.clock.elapsed;

	double RunningTime = 0;
	short FrameCount = 0;
	double TargetFrameSeconds = 1.0f / 60;

	UL_DEBUG(Memory::GetMemoryUsageStr());

	while (AppState.is_running) {
		if (!Platform::PlatformPumpMessage(&AppState.platform)) {
			AppState.is_running = false;
		}

		if (!AppState.is_suspended) {
			Clock::Update(&AppState.clock);
			double CurrentTime = AppState.clock.elapsed;
			double DeltaTime = (CurrentTime - AppState.last_time);
			double FrameStartTime = Platform::PlatformGetAbsoluteTime();

			if (!AppState.game_instance->update(AppState.game_instance, (float)DeltaTime)) {
				UL_FATAL("Game update failed!");
				AppState.is_running = false;
				break;
			}

			if (!AppState.game_instance->render(AppState.game_instance, (float)DeltaTime)) {
				UL_FATAL("Game render failed!");
				AppState.is_running = false;
				break;
			}

			size_t MeshCount = AppState.Meshes.size();
			if (MeshCount > 0) {
				// Perform a small rotation on the first mesh.
				Quaternion Rotation = QuaternionFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), 0.5f * (float)DeltaTime, false);
				AppState.Meshes[0].Transform.Rotate(Rotation);

				if (MeshCount > 1) {
					AppState.Meshes[1].Transform.Rotate(Rotation);
				}

				if (MeshCount > 2) {
					AppState.Meshes[2].Transform.Rotate(Rotation);
				}
			}

			// TODO: Refactor packet creation.
			SRenderPacket Packet;
			Packet.delta_time = DeltaTime;

			// TODO: Read from config.
			Packet.view_count = 3;
			std::vector<RenderViewPacket> Views;
			Views.resize(Packet.view_count);
			Packet.views = Views;

			// Skybox
			SkyboxPacketData SkyboxData;
			SkyboxData.sb = &AppState.SB;
			if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("Skybox"), &SkyboxData, &Packet.views[0])) {
				UL_ERROR("Failed to build packet for view 'World_Opaque'.");
				return false;
			}

			// World
			MeshPacketData WorldMeshData;
			WorldMeshData.mesh_count = (uint32_t)AppState.Meshes.size();
			WorldMeshData.meshes = AppState.Meshes;
			if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("World_Opaque"), &WorldMeshData, &Packet.views[1])) {
				UL_ERROR("Failed to build packet for view 'World_Opaque'.");
				return false;
			}

			// UI
			MeshPacketData UIMeshData;
			UIMeshData.mesh_count = (uint32_t)AppState.UIMeshes.size();
			UIMeshData.meshes = AppState.UIMeshes;
			if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("UI"), &UIMeshData, &Packet.views[2])) {
				UL_ERROR("Failed to build packet for view 'UI'.");
				return false;
			}

			Renderer->DrawFrame(&Packet);

			// Figure FPS
			double FrameEndTime = Platform::PlatformGetAbsoluteTime();
			double FrameEsapsedTime = FrameEndTime - FrameStartTime;
			RunningTime += FrameEsapsedTime;
			double RemainingSceonds = TargetFrameSeconds - FrameEsapsedTime;

			// Limit FPS
			if (RemainingSceonds > 0) {
				int RemainingMS = static_cast<int>(RemainingSceonds * 1000);
				bool LimitFrames = false;
				if (RemainingMS > 0 && LimitFrames) {
					Platform::PlatformSleep(RemainingMS - 1);
				}

				FrameCount++;
			}

			Core::InputUpdate(DeltaTime);

			// Update time
			AppState.last_time = CurrentTime;
		}
	}

	AppState.is_running = false;

	// Shutdown event system
	Core::EventUnregister(Core::eEvent_Code_Application_Quit, 0, ApplicationOnEvent);
	Core::EventUnregister(Core::eEvent_Code_Key_Pressed, 0, ApplicationOnKey);
	Core::EventUnregister(Core::eEvent_Code_Key_Released, 0, ApplicationOnKey);

	Core::EventShutdown();
	Core::InputShutdown();

	CameraSystem::Shutdown();
	GeometrySystem::Shutdown();
	MaterialSystem::Shutdown();
	TextureSystem::Shutdown();
	ShaderSystem::Shutdown();

	// Temp
	Renderer->ReleaseTextureMap(&AppState.SB.Cubemap);

	RenderViewSystem::Shutdown();
	ResourceSystem::Shutdown();

	Renderer->Shutdown();
	Memory::Free(Renderer, sizeof(IRenderer), MemoryType::eMemory_Type_Renderer);

	Platform::PlatformShutdown(&AppState.platform);

	return true;
}

bool ApplicationOnEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	switch (code){
	case Core::eEvent_Code_Application_Quit: {
		UL_INFO("Application quit now.");
		AppState.is_running = false;
		return true;
	}
	}

	return false;
}

bool ApplicationOnKey(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
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
	
	return false;
}

bool ApplicationOnResized(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	if (Renderer == nullptr) {
		return false;
	}

	if (code == Core::eEvent_Code_Resize) {
		unsigned short Width = context.data.u16[0];
		unsigned short Height = context.data.u16[1];

		//Check if different. If so, trigger a resize event.
		if (Width != AppState.width || Height != AppState.height) {
			AppState.width = Width;
			AppState.height = Height;
			UL_DEBUG("Window resize: %i %i", Width, Height);

			// Handle minimization
			if (Width == 0 || Height == 0) {
				UL_INFO("Window minimized, suspending application.");
				AppState.is_suspended = true;
				return true;
			}
			else {
				if (AppState.is_suspended) {
					UL_INFO("Window restored, resuming application.");
					AppState.is_suspended = false;
				}

				// TODO: Temp
				SGeometryConfig UIConfig;
				UIConfig.vertex_size = sizeof(Vertex2D);
				UIConfig.vertex_count = 4;
				UIConfig.index_size = sizeof(uint32_t);
				UIConfig.index_count = 6;
				strncpy(UIConfig.material_name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);
				strncpy(UIConfig.name, "Material.UI", MATERIAL_NAME_MAX_LENGTH);

				const float h = 256.0f;
				const float w = 64.0f;
				const float x = 0.0f;
				const float y = Height;

				Vertex2D UIVerts[4];
				UIVerts[0].position.x = x;
				UIVerts[0].position.y = y;
				UIVerts[0].texcoord.x = 0.0f;
				UIVerts[0].texcoord.y = 0.0f;

				UIVerts[1].position.x = x + h;
				UIVerts[1].position.y = y - w;
				UIVerts[1].texcoord.x = 1.0f;
				UIVerts[1].texcoord.y = 1.0f;

				UIVerts[2].position.x = x;
				UIVerts[2].position.y = y - w;
				UIVerts[2].texcoord.x = 0.0f;
				UIVerts[2].texcoord.y = 1.0f;

				UIVerts[3].position.x = x + h;
				UIVerts[3].position.y = y;
				UIVerts[3].texcoord.x = 1.0f;
				UIVerts[3].texcoord.y = 0.0f;

				UIConfig.vertices = UIVerts;

				// Indices
				uint32_t UIIndices[6] = { 0, 2, 1, 0, 1, 3 };
				UIConfig.indices = UIIndices;

				AppState.UIMeshes[0].geometries[0] = GeometrySystem::AcquireFromConfig(UIConfig, true);
				AppState.game_instance->on_resize(AppState.game_instance, Width, Height);
				Renderer->OnResize(Width, Height);
			}
		}
	}

	return false;
}

void GetFramebufferSize(unsigned int* width, unsigned int* height) {
	*width = AppState.width;
	*height = AppState.height;
}