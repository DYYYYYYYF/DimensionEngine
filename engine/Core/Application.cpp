#include "Application.hpp"

#include "EngineLogger.hpp"
#include "Event.hpp"
#include "Input.hpp"
#include "DMemory.hpp"
#include "Clock.hpp"

#include "GameType.hpp"
#include "Platform/Platform.hpp"

struct SApplicationState {
	SGame* game_instance;
	SPlatformState platform;

	bool is_running;
	bool is_suspended;
	short width;
	short height;
	double last_time;
	SClock clock;
};

static bool Initialized = false;
static SApplicationState AppState;

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
	
	Platform::PlatformStartup(&AppState.platform,
		game_instance->app_config.name, 
		game_instance->app_config.start_x, 
		game_instance->app_config.start_y, 
		game_instance->app_config.start_width, 
		game_instance->app_config.start_height);

	// Init Game
	if (!AppState.game_instance->initialize(AppState.game_instance)) {
		UL_FATAL("Game failed to initialize!");
		return false;
	}

	AppState.game_instance->on_resize(AppState.game_instance, AppState.width, AppState.height);

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