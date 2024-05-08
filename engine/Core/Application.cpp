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

struct SApplicationState {
	SGame* game_instance;
	SPlatformState platform;

	bool is_running;
	bool is_suspended;
	short width;
	short height;
	double last_time;
	SClock clock;

	Geometry* TestGeometry;
	Geometry* TestUIGeometry;
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

	UL_INFO("Test Info");
	UL_DEBUG("Test Debug");
	UL_ERROR("Test Error");
	UL_WARN("Test Warn");
	UL_FATAL("Test Fatal");


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
	ResourceSystemConfig.asset_base_path = "../Asset";
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

	// TODO: Temp
	//AppState.TestGeometry = GeometrySystem::GetDefaultGeometry();
	SGeometryConfig GeoConfig = GeometrySystem::GeneratePlaneConfig(5.0f, 2.0f, 5, 2, 5.0f, 2.0f, "TestGeometry", "TestMaterial");
	AppState.TestGeometry = GeometrySystem::AcquireFromConfig(GeoConfig, true);

	// Clean up the allocations for the geometry config.
	Memory::Free(GeoConfig.vertices, sizeof(Vertex) * GeoConfig.vertex_count, MemoryType::eMemory_Type_Array);
	Memory::Free(GeoConfig.indices, sizeof(uint32_t) * GeoConfig.index_count, MemoryType::eMemory_Type_Array);

	// Load up some test UI geometry.
	SGeometryConfig UIConfig;
	UIConfig.vertex_size = sizeof(Vertex2D);
	UIConfig.vertex_count = 4;
	UIConfig.index_size = sizeof(uint32_t);
	UIConfig.index_count = 6;
	strncpy(UIConfig.material_name, "TestUIMaterial", MATERIAL_NAME_MAX_LENGTH);
	strncpy(UIConfig.name, "TestUIMaterial", MATERIAL_NAME_MAX_LENGTH);

	const float f = 512.0f;
	Vertex2D UIVerts[4];
	UIVerts[0].position.x = 0.0f;
	UIVerts[0].position.y = 0.0f;
	UIVerts[0].texcoord.x = 0.0f;
	UIVerts[0].texcoord.y = 0.0f;

	UIVerts[1].position.x = f;
	UIVerts[1].position.y = f;
	UIVerts[1].texcoord.x = 1.0f;
	UIVerts[1].texcoord.y = 1.0f;

	UIVerts[2].position.x = 0.0f;
	UIVerts[2].position.y = f;
	UIVerts[2].texcoord.x = 0.0f;
	UIVerts[2].texcoord.y = 1.0f;

	UIVerts[3].position.x = f;
	UIVerts[3].position.y = 0.0f;
	UIVerts[3].texcoord.x = 1.0f;
	UIVerts[3].texcoord.y = 0.0f;

	UIConfig.vertices = UIVerts;

	// Indices
	uint32_t UIIndices[6] = { 2, 1, 0, 1, 3, 0 };
	UIConfig.indices = UIIndices;

	// Get UI geometry from config.
	AppState.TestUIGeometry = GeometrySystem::AcquireFromConfig(UIConfig, true);
	// AppState.TestUIGeometry = GeometrySystem::GetDefaultGeometry2D();

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

			if (!AppState.game_instance->update(AppState.game_instance, (float)0)) {
				UL_FATAL("Game update failed!");
				AppState.is_running = false;
				break;
			}

			if (!AppState.game_instance->render(AppState.game_instance, (float)0)) {
				UL_FATAL("Game render failed!");
				AppState.is_running = false;
				break;
			}

			// TODO: Refactor packet
			SRenderPacket Packet;
			Packet.delta_time = DeltaTime;

			// TODO: Temp
			GeometryRenderData TestRender;
			TestRender.geometry = AppState.TestGeometry;
			TestRender.model = Matrix4::Identity();
			Packet.geometries = &TestRender;
			Packet.geometry_count = 1;

			GeometryRenderData TestUIRender;
			TestUIRender.geometry = AppState.TestUIGeometry;
			TestUIRender.model = Matrix4::Identity();
			Packet.ui_geometries = &TestUIRender;
			Packet.ui_geometry_count = 1;

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

	ResourceSystem::Shutdown();
	GeometrySystem::Shutdown();
	MaterialSystem::Shutdown();
	TextureSystem::Shutdown();

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
		else if (KeyCode == eKeys_A) {
			//printf("Key 'A' pressed!");
		}
		else {
			//printf("%c key released.", KeyCode);
		}
	}
	else if (code == Core::eEvent_Code_Key_Released) {
		unsigned short KeyCode = context.data.u16[0];
		if (KeyCode == eKeys_B) {
			//printf("B pressed!");
		}
		else {
			//printf("%c released!", KeyCode);
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