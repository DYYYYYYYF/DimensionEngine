#include "Engine.hpp"

#include "EngineLogger.hpp"
#include "Event.hpp"
#include "Controller.hpp"
#include "DMemory.hpp"
#include "Clock.hpp"
#include "Metrics.hpp"

#include "IGame.hpp"
#include "Platform/Platform.hpp"

#include "Rendering/Renderer.hpp"
#include "Rendering/Interface/IRenderpass.hpp"
#include "Math/MathTypes.hpp"
#include "Containers/TString.hpp"

// Systems
#include "Systems/TextureSystem.h"
#include "Systems/MaterialSystem.h"
#include "Systems/GeometrySystem.h"
#include "Systems/ResourceSystem.h"
#include "Systems/ShaderSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderViewSystem.hpp"
#include "Systems/JobSystem.hpp"
#include "Systems/FontSystem.hpp"
#include "Utils/FileWatcher.h"

bool Engine::Initialize(){
	if (Initialized) {
		GLOG(Log::eError, "Create application more than once!");
		return false;
	}

	if (GameInst == nullptr) {
		GLOG(Log::eError, "Create application failed! Game instance is nullptr!");
		return false;
	}

	// Controller
	Controller::Initialize();

	// Metrics
	Metrics::Initialize();

	is_running = true;
	is_suspended = false;

	// Event
	if (!EngineEvent::Initialize()) {
		GLOG(Log::eError, "Event system init failed. Application can not start.");
		return false;
	}

	// Register for engine-level events.
	EngineEvent::Register(eEventCode::Application_Quit, nullptr,
		std::bind(&Engine::OnEvent, this, std::placeholders::_1, this, std::placeholders::_3, std::placeholders::_4));
	EngineEvent::Register(eEventCode::Resize, nullptr,
		std::bind(&Engine::OnResized, this, std::placeholders::_1, this, std::placeholders::_3, std::placeholders::_4));

	// Platform
	if (!Platform::PlatformStartup(&platform,
		GameInst->GetApplicationName(),
		(int)GameInst->GetWindowOffsetX(),
		(int)GameInst->GetWindowOffsetY(),
		GameInst->GetWindowWidth(),
		GameInst->GetWindowHeight())) {
        GLOG(Log::eFatal, "Failed to startup platform. Application quit now!");
		return false;
    }

	width = (short)GameInst->GetWindowWidth();
	height = (short)GameInst->GetWindowHeight();

	// Init texture system
	SResourceSystemConfig ResourceSystemConfig;
	ResourceSystemConfig.max_loader_count = 32;
    ResourceSystemConfig.asset_base_path = std::string(ROOT_PATH) + "/Assets";

	if (!ResourceSystem::Initialize(ResourceSystemConfig)) {
		GLOG(Log::eFatal, "Resource system failed to initialize!");
		return false;
	}

	// Init Renderer
	if (Renderer == nullptr) {
		void* TempRenderer = (IRenderer*)Memory::Allocate(sizeof(IRenderer), MemoryType::eMemory_Type_Renderer);
		Renderer = new(TempRenderer)IRenderer(eRenderer_Backend_Type_Vulkan, &platform);
		ASSERT(Renderer);
	}

	// Init shader system
	ShaderSystem::Config ShaderSystemConfig;
	ShaderSystemConfig.max_shader_count = 1024;
	ShaderSystemConfig.max_uniform_count = 128;
	ShaderSystemConfig.max_global_textures = 31;
	ShaderSystemConfig.max_instance_textures = 31;
	if (!ShaderSystem::Initialize(Renderer, ShaderSystemConfig)) {
		GLOG(Log::eFatal, "Shader system failed to initialize!");
		return false;
	}

	// This is really a core count. Subtract 1 to account for the main thread already being in use.
	bool RenderWithMultithread = Renderer->GetEnabledMutiThread();
	int ThreadCount = Platform::GetProcessorCount() - 1;
	if (ThreadCount < 1) {
		GLOG(Log::eFatal, "Error: Platform reported processor count (minus one for main thread) as %i. Need at least one additional thread for the job system.", ThreadCount);
	}
	else {
		GLOG(Log::eInfo, "Available threads: %i.", ThreadCount);
	}

	// Cap the thread count.
	const int MaxThreadCount = 15;
	if (ThreadCount > MaxThreadCount) {
		GLOG(Log::eInfo, "Available threads on the system is %i, but will be capped at %i.", ThreadCount, MaxThreadCount);
		ThreadCount = MaxThreadCount;
	}

	// Initialize the job system.
	// Requires knowledge of renderer multithread support, so should be initialized here.
	uint32_t JobThreadTypes[15];
	for (uint32_t i = 0; i < 15; ++i) {
		JobThreadTypes[i] = (uint32_t)JobType::eGeneral;
	}

	if (ThreadCount == 1 || !RenderWithMultithread) {
		// Everything on one job thread.
		JobThreadTypes[0] |= ((uint32_t)JobType::eGPU_Resource | (uint32_t)JobType::eResource_Load);
	}
	else if (ThreadCount == 2) {
		// Split things between 2 threads.
		JobThreadTypes[0] |= (uint32_t)JobType::eGPU_Resource;
		JobThreadTypes[1] |= (uint32_t)JobType::eResource_Load;
	}
	else {
		// Dedicate the first 2 threads to these thing, pass of general tasks to other threads.
		JobThreadTypes[0] = (uint32_t)JobType::eGPU_Resource;
		JobThreadTypes[1] = (uint32_t)JobType::eResource_Load;
	}

	// Job system
	if (!JobSystem::Initialize(ThreadCount, JobThreadTypes)) {
		GLOG(Log::eFatal, "Job system failed to initialize!");
		return false;
	}

	// Render system.
	if (!Renderer->Initialize(GameInst->GetApplicationName(),Vector2(width, height), & platform)) {
		GLOG(Log::eFatal, "Renderer failed to initialize!");
		return false;
	}

	// Perform the game's boot sequence.
	if (!GameInst->Boot(Renderer)) {
		GLOG(Log::eFatal, "Game boot sequence failed!");
		return false;
	}

	// Init texture system
	STextureSystemConfig TextureSystemConfig;
	TextureSystemConfig.max_texture_count = 65536;
	if (!TextureSystem::Get().Initialize(Renderer, TextureSystemConfig)) {
		GLOG(Log::eFatal, "Texture system failed to initialize!");
		return false;
	}

	// Init camera system
	SCameraSystemConfig CameraSystemConfig;
	CameraSystemConfig.max_camera_count = 61;
	if (!CameraSystem::Initialize(Renderer, CameraSystemConfig)) {
		GLOG(Log::eFatal, "Camera system failed to initialize!");
		return false;
	}

	// Init font system.
	if (!FontSystem::Get().Initialize(Renderer, GameInst->GetFontConfig())) {
		GLOG(Log::eFatal, "Font system failed to initialize!");
		return false;
	}

	// Init render view system.
	SRenderViewSystemConfig RenderViewSysConfig;
	RenderViewSysConfig.max_view_count = 255;
	if (!RenderViewSystem::Initialize(Renderer, RenderViewSysConfig)) {
		GLOG(Log::eFatal, "Render view system failed to intialize!");
		return false;
	}

	// Load render views from app config.
	const std::vector<RenderViewConfig>& Renderviews = GameInst->GetRenderviews();
	uint32_t ViewCount = (uint32_t)Renderviews.size();
	for (uint32_t v = 0; v < ViewCount; ++v) {
		const RenderViewConfig& View = Renderviews[v];
		if (!RenderViewSystem::Create(View)) {
			GLOG(Log::eFatal, "Failed to create view '%s'.", View.name);
			return false;
		}
	}

	// Init material system
	SMaterialSystemConfig MaterialSystemConfig;
	MaterialSystemConfig.max_material_count = 4096;
	if (!MaterialSystem::Initialize(Renderer, MaterialSystemConfig)) {
		GLOG(Log::eFatal, "Material system failed to initialize!");
		return false;
	}

	// Init geometry system
	if (!GeometrySystem::Get().Initialize(Renderer)) {
		GLOG(Log::eFatal, "Geometry system failed to initialize!");
		return false;
	}

	// Init Game
	if (!GameInst->Initialize()) {
		GLOG(Log::eFatal, "Game failed to initialize!");
		return false;
	}

	GameInst->OnResize(width, height);
	Renderer->OnResize(width, height);

	Initialized = true;
	return true;
}

static FileWatcher* GlobalFileWatcher = nullptr;

bool Engine::Run() {
	AppClock.Start();
	AppClock.Update();
	last_time = AppClock.GetElapsedTime();	// Seconds

	short FrameCount = 0;
	double FrameElapsedTime = 0.0;
	double TargetFrameSeconds = 1.0 / 120.0;

	GLOG(Log::eDebug, Memory::GetMemoryUsageStr());

	GlobalFileWatcher = NewObject<FileWatcher>();

	if (ShaderSystem::GLOBAL_SHADER_TYPE == EShaderLanguage::eGLSL) {
		GlobalFileWatcher->AddWatchFolder("../Shaders/glsl/");
	}
	else {
		GlobalFileWatcher->AddWatchFolder("../Shaders/hlsl/");
	}

	while (is_running) {
		if (!Platform::PlatformPumpMessage(&platform)) {
			is_running = false;
		}

		if (!is_suspended) {
			AppClock.Update();
			double CurrentTime = AppClock.GetElapsedTime();		// Seconds
			double DeltaTime = (CurrentTime - last_time);
			double FrameStartTime = Platform::PlatformGetAbsoluteTime();

			// Detective file status.
			GlobalFileWatcher->Update();

			// Update Job system.
			JobSystem::Update();

			// Update metrics.
			Metrics::Update(FrameElapsedTime);

			if (!GameInst->Update((float)DeltaTime)) {
				GLOG(Log::eFatal, "Game update failed!");
				is_running = false;
				break;
			}
			GameController->Update(DeltaTime);

			// TODO: Refactor packet creation.
			SRenderPacket Packet;
			Packet.delta_time = DeltaTime;

			// Call the game's render routine.
			if (!GameInst->Render(&Packet, (float)DeltaTime)) {
				GLOG(Log::eFatal, "Game render faield. shutting down.");
				is_running = false;
				break;
			}

			Renderer->DrawFrame(&Packet);

			// Cleanup the packet.
			for (uint32_t i = 0; i < (uint32_t)Packet.views.size(); ++i) {
				IRenderView* RenderView = Packet.views[i].view;
				if (RenderView) {
					RenderView->OnDestroyPacket(&Packet.views[i]);
				}
			}
			Packet.views.clear();
			std::vector<struct RenderViewPacket>().swap(Packet.views);

			double FrameEndTime = Platform::PlatformGetAbsoluteTime();
			FrameElapsedTime = FrameEndTime - FrameStartTime;
			double RemainingSceonds = TargetFrameSeconds - FrameElapsedTime;

			// Limit FPS
			if (RemainingSceonds > 0) {
				int RemainingMS = static_cast<int>(RemainingSceonds * 1000);
				// TODO: Configurable
				bool LimitFrames = false;
				if (RemainingMS > 0 && LimitFrames) {
					Platform::PlatformSleep(RemainingMS - 1);
				}

				FrameCount++;
			}

			last_time = CurrentTime;
			GameInst->DeltaTime = (float)DeltaTime;
		}
	}

	is_running = false;

	// Shut down the game.
	GameInst->Shutdown();

	// Shutdown event system
	EngineEvent::Unregister(eEventCode::Application_Quit, nullptr,
		std::bind(&Engine::OnEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	EngineEvent::Unregister(eEventCode::Resize, nullptr,
		std::bind(&Engine::OnResized, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	RenderViewSystem::Shutdown();
	CameraSystem::Shutdown();
	FontSystem::Get().Shutdown();
	GeometrySystem::Get().Shutdown();
	MaterialSystem::Shutdown();
	TextureSystem::Get().Shutdown();
	JobSystem::Shutdown();
	ShaderSystem::Shutdown();

	Renderer->Shutdown();
	Memory::Free(Renderer, MemoryType::eMemory_Type_Renderer);

	EngineEvent::Shutdown();
	Controller::Shutdown();
	ResourceSystem::Shutdown();
	Platform::PlatformShutdown(&platform);

	return true;
}

bool Engine::OnEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
	(void)context;
	(void)listener_instance;
	(void)sender;

	switch (code){
	case eEventCode::Application_Quit: {
		GLOG(Log::eInfo, "Application quit now.");
		is_running = false;
		return true;
	}
        default: break;
	}	// Switch

	return false;
}

bool Engine::OnResized(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
	(void)listener_instance;
	(void)sender;

	if (Renderer == nullptr) {
		return false;
	}

	if (code == eEventCode::Resize) {
		unsigned short Width = context.data.u16[0];
		unsigned short Height = context.data.u16[1];

		//Check if different. If so, trigger a resize event.
		if (Width != width || Height != height) {
			width = Width;
			height = Height;
			GLOG(Log::eDebug, "Window resize: %i %i", Width, Height);

			// Handle minimization
			if (Width == 0 || Height == 0) {
				GLOG(Log::eInfo, "Window minimized, suspending application.");
				is_suspended = true;
				return true;
			}
			else {
				if (is_suspended) {
					GLOG(Log::eInfo, "Window restored, resuming application.");
					is_suspended = false;
				}

				GameInst->OnResize(Width, Height);
				Renderer->OnResize(Width, Height);

				return true;
			}
		}
	}

	return true;
}

void Engine::GetFramebufferSize(unsigned int* w, unsigned int* h) {
	*w = this->width;
	*h = this->height;
}
