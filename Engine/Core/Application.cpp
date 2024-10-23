#include "Application.hpp"

#include "EngineLogger.hpp"
#include "Event.hpp"
#include "Input.hpp"
#include "DMemory.hpp"
#include "Clock.hpp"
#include "UID.hpp"
#include "Metrics.hpp"

#include "GameType.hpp"
#include "Platform/Platform.hpp"

#include "Renderer/RendererFrontend.hpp"
#include "Renderer/Interface/IRenderpass.hpp"
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

bool Application::Initialize(){
	if (Initialized) {
		LOG_ERROR("Create application more than once!");
		return false;
	}

	if (GameInst == nullptr) {
		LOG_ERROR("Create application failed! Game instance is nullptr!");
		return false;
	}

	UID::Seed(101);

	// Controller
	Controller::Initialize();

	// Metrics
	Metrics::Initialize();

	is_running = true;
	is_suspended = false;

	// Input
	if (!EngineEvent::Initialize()) {
		LOG_ERROR("Event system init failed. Application can not start.");
		return false;
	}

	// Register for engine-level events.
	EngineEvent::Register(eEventCode::Application_Quit, nullptr,
		std::bind(&Application::OnEvent, this, std::placeholders::_1, this, std::placeholders::_3, std::placeholders::_4));
	EngineEvent::Register(eEventCode::Resize, nullptr,
		std::bind(&Application::OnResized, this, std::placeholders::_1, this, std::placeholders::_3, std::placeholders::_4));

	// Platform
	if (!Platform::PlatformStartup(&platform,
		GameInst->AppConfig.name,
		GameInst->AppConfig.start_x, 
		GameInst->AppConfig.start_y, 
		GameInst->AppConfig.start_width, 
        GameInst->AppConfig.start_height)){
        LOG_FATAL("Failed to startup platform. Application quit now!");
		return false;
    }

	width = GameInst->AppConfig.start_width;
	height = GameInst->AppConfig.start_height;

	// Init texture system
	SResourceSystemConfig ResourceSystemConfig;
	ResourceSystemConfig.max_loader_count = 32;
#ifdef DPLATFORM_WINDOWS
	ResourceSystemConfig.asset_base_path = "../Assets";
#elif DPLATFORM_MACOS
    ResourceSystemConfig.asset_base_path = "../../Assets";
#endif
	if (!ResourceSystem::Initialize(ResourceSystemConfig)) {
		LOG_FATAL("Resource system failed to initialize!");
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
		LOG_FATAL("Shader system failed to initialize!");
		return false;
	}

	// This is really a core count. Subtract 1 to account for the main thread already being in use.
	bool RenderWithMultithread = Renderer->GetEnabledMutiThread();
	int ThreadCount = Platform::GetProcessorCount() - 1;
	if (ThreadCount < 1) {
		LOG_FATAL("Error: Platform reported processor count (minus one for main thread) as %i. Need at least one additional thread for the job system.", ThreadCount);
	}
	else {
		LOG_INFO("Available threads: %i.", ThreadCount);
	}

	// Cap the thread count.
	const int MaxThreadCount = 15;
	if (ThreadCount > MaxThreadCount) {
		LOG_INFO("Available threads on the system is %i, but will be capped at %i.", ThreadCount, MaxThreadCount);
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
		LOG_FATAL("Job system failed to initialize!");
		return false;
	}

	// Render system.
	if (!Renderer->Initialize(GameInst->AppConfig.name, &platform)) {
		LOG_FATAL("Renderer failed to initialize!");
		return false;
	}

	// Perform the game's boot sequence.
	if (!GameInst->Boot(Renderer)) {
		LOG_FATAL("Game boot sequence failed!");
		return false;
	}

	// Init texture system
	STextureSystemConfig TextureSystemConfig;
	TextureSystemConfig.max_texture_count = 65536;
	if (!TextureSystem::Initialize(Renderer, TextureSystemConfig)) {
		LOG_FATAL("Texture system failed to initialize!");
		return false;
	}

	// Init camera system
	SCameraSystemConfig CameraSystemConfig;
	CameraSystemConfig.max_camera_count = 61;
	if (!CameraSystem::Initialize(Renderer, CameraSystemConfig)) {
		LOG_FATAL("Camera system failed to initialize!");
		return false;
	}

	// Init font system.
	if (!FontSystem::Initialize(Renderer, &GameInst->AppConfig.FontConfig)) {
		LOG_FATAL("Font system failed to initialize!");
		return false;
	}

	// Init render view system.
	SRenderViewSystemConfig RenderViewSysConfig;
	RenderViewSysConfig.max_view_count = 255;
	if (!RenderViewSystem::Initialize(Renderer, RenderViewSysConfig)) {
		LOG_FATAL("Render view system failed to intialize!");
		return false;
	}

	// Load render views from app config.
	uint32_t ViewCount = (uint32_t)GameInst->AppConfig.Renderviews.size();
	for (uint32_t v = 0; v < ViewCount; ++v) {
		const RenderViewConfig& View = GameInst->AppConfig.Renderviews[v];
		if (!RenderViewSystem::Create(View)) {
			LOG_FATAL("Failed to create view '%s'.", View.name);
			return false;
		}
	}

	// Init material system
	SMaterialSystemConfig MaterialSystemConfig;
	MaterialSystemConfig.max_material_count = 4096;
	if (!MaterialSystem::Initialize(Renderer, MaterialSystemConfig)) {
		LOG_FATAL("Material system failed to initialize!");
		return false;
	}

	// Init geometry system
	SGeometrySystemConfig GeometrySystemConfig;
	GeometrySystemConfig.max_geometry_count = 4096;
	if (!GeometrySystem::Initialize(Renderer, GeometrySystemConfig)) {
		LOG_FATAL("Geometry system failed to initialize!");
		return false;
	}

	// Init Game
	if (!GameInst->Initialize()) {
		LOG_FATAL("Game failed to initialize!");
		return false;
	}

	GameInst->OnResize(width, height);
	Renderer->OnResize(width, height);

	Initialized = true;
	return true;
}

bool Application::Run() {
	AppClock.Start();
	AppClock.Update();
	last_time = AppClock.GetElapsedTime();	// Seconds

	short FrameCount = 0;
	double FrameElapsedTime = 0.0;
	double TargetFrameSeconds = 1.0 / 120.0;

	LOG_DEBUG(Memory::GetMemoryUsageStr());

	while (is_running) {
		if (!Platform::PlatformPumpMessage(&platform)) {
			is_running = false;
		}

		if (!is_suspended) {
			AppClock.Update();
			double CurrentTime = AppClock.GetElapsedTime();		// Seconds
			double DeltaTime = (CurrentTime - last_time);
			double FrameStartTime = Platform::PlatformGetAbsoluteTime();

			// Update Job system.
			JobSystem::Update();

			// Update metrics.
			Metrics::Update(FrameElapsedTime);

			if (!GameInst->Update((float)DeltaTime)) {
				LOG_FATAL("Game update failed!");
				is_running = false;
				break;
			}

			// TODO: Refactor packet creation.
			SRenderPacket Packet;
			Packet.delta_time = DeltaTime;

			// Call the game's render routine.
			if (!GameInst->Render(&Packet, (float)DeltaTime)) {
				LOG_FATAL("Game render faield. shutting down.");
				is_running = false;
				break;
			}

			Renderer->DrawFrame(&Packet);

			// Cleanup the packet.
			for (uint32_t i = 0; i < Packet.view_count; ++i) {
				IRenderView* RenderView = Packet.views[i].view;
				RenderView->OnDestroyPacket(&Packet.views[i]);
			}

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
			GameController->Update(DeltaTime);
		}
	}

	is_running = false;

	// Shut down the game.
	GameInst->Shutdown();

	// Shutdown event system
	EngineEvent::Unregister(eEventCode::Application_Quit, nullptr,
		std::bind(&Application::OnEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	EngineEvent::Unregister(eEventCode::Resize, nullptr,
		std::bind(&Application::OnResized, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	RenderViewSystem::Shutdown();
	CameraSystem::Shutdown();
	FontSystem::Shutdown();
	GeometrySystem::Shutdown();
	MaterialSystem::Shutdown();
	TextureSystem::Shutdown();
	JobSystem::Shutdown();
	ShaderSystem::Shutdown();

	Renderer->Shutdown();
	Memory::Free(Renderer, sizeof(IRenderer), MemoryType::eMemory_Type_Renderer);

	EngineEvent::Shutdown();
	Controller::Shutdown();
	ResourceSystem::Shutdown();
	Platform::PlatformShutdown(&platform);

	return true;
}

bool Application::OnEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
	switch (code){
	case eEventCode::Application_Quit: {
		LOG_INFO("Application quit now.");
		is_running = false;
		return true;
	}
        default: break;
	}	// Switch

	return false;
}

bool Application::OnResized(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
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
			LOG_DEBUG("Window resize: %i %i", Width, Height);

			// Handle minimization
			if (Width == 0 || Height == 0) {
				LOG_INFO("Window minimized, suspending application.");
				is_suspended = true;
				return true;
			}
			else {
				if (is_suspended) {
					LOG_INFO("Window restored, resuming application.");
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

void Application::GetFramebufferSize(unsigned int* width, unsigned int* height) {
	*width = this->width;
	*height = this->height;
}
