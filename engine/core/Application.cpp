#include "Application.hpp"

#include "EngineLogger.hpp"
#include "Event.hpp"
#include "Input.hpp"
#include "DMemory.hpp"

#include "../GameType.hpp"
#include "../platform/platform.hpp"

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
	Input::InputInitialize();

	UL_INFO("Test Info");
	UL_DEBUG("Test Debug");
	UL_ERROR("Test Error");
	UL_WARN("Test Warn");
	UL_FATAL("Test Fatal");


	AppState.is_running = true;
	AppState.is_suspended = false;

	if (!Event::EventInitialize()) {
		UL_ERROR("Event system init failed. Application can not start.");
		return false;
	}

	Event::EventRegister(Event::eEvent_Code_Application_Quit, 0, ApplicationOnEvent);
	Event::EventRegister(Event::eEvent_Code_Key_Pressed, 0, ApplicationOnKey);
	Event::EventRegister(Event::eEvent_Code_Key_Released, 0, ApplicationOnKey);
	
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
	UL_DEBUG(Memory::GetMemoryUsageStr());
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

		Input::InputUpdate(0);
	}

	AppState.is_running = false;

	// Shutdown event system
	Event::EventUnregister(Event::eEvent_Code_Application_Quit, 0, ApplicationOnEvent);
	Event::EventUnregister(Event::eEvent_Code_Key_Pressed, 0, ApplicationOnKey);
	Event::EventUnregister(Event::eEvent_Code_Key_Released, 0, ApplicationOnKey);

	Event::EventShutdown();
	Input::InputShutdown();

	Platform::PlatformShutdown(&AppState.platform);

	return true;
}

bool ApplicationOnEvent(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	switch (code){
	case Event::eEvent_Code_Application_Quit: {
		UL_INFO("Application quit now.");
		AppState.is_running = false;
		return true;
	}
	}

	return false;
}

bool ApplicationOnKey(unsigned short code, void* sender, void* listener_instance, SEventContext context) {
	if (code == Event::eEvent_Code_Key_Pressed) {
		unsigned short KeyCode = context.data.u16[0];
		if (KeyCode == eKeys_Escape) {
			// NOTE: Technically dispatch an event to itself, but there may be other listeners.
			SEventContext data = {};
			Event::EventFire(Event::eEvent_Code_Application_Quit, 0, data);

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
	else if (code == Event::eEvent_Code_Key_Released) {
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