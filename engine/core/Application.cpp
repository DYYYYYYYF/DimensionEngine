#include "Application.hpp"
#include "../GameType.hpp"
#include "../platform/platform.hpp"
#include "DMemory.hpp"

struct SApplicationState {
	SGame* game_instance;
	SPlatformState platform;

	bool is_running;
	bool is_suspended;
	short width;
	short height;
	double last_time;
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
	UL_INFO("Test Info");
	UL_DEBUG("Test Debug");
	UL_ERROR("Test Error");
	UL_WARN("Test Warn");
	UL_FATAL("Test Fatal");


	AppState.is_running = true;
	AppState.is_suspended = false;

	Platform::PlatformStartup(&AppState.platform,
		game_instance->app_config.name, 
		game_instance->app_config.start_x, 
		game_instance->app_config.start_y, 
		game_instance->app_config.start_width, 
		game_instance->app_config.start_height);

	if (!AppState.game_instance->initialize(AppState.game_instance)) {
		UL_FATAL("Game failed to initialize!");
		return false;
	}

	AppState.game_instance->on_resize(AppState.game_instance, AppState.width, AppState.height);

	Initialized = true;
	return true;
}

bool ApplicationRun() {
	UL_INFO(Memory::GetMemoryUsageStr());
	while (AppState.is_running) {
		if (!Platform::PlatformPumpMessage(&AppState.platform)) {
			AppState.is_running = false;
		}

		if (AppState.is_suspended) {
			if (!AppState.game_instance->update(AppState.game_instance, (float)0)) {
				UL_FATAL("Game update failed!");
				AppState.is_running = false;
				break;
			}
		}

		if (!AppState.game_instance->render(AppState.game_instance, (float)0)) {
			UL_FATAL("Game render failed!");
			AppState.is_running = false;
			break;
		}
	}

	AppState.is_running = false;
	Platform::PlatformShutdown(&AppState.platform);

	return true;
}