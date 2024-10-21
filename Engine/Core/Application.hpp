#pragma once

#include "Defines.hpp"
#include "Clock.hpp"
#include "Event.hpp"
#include "Resources/Skybox.hpp"
#include "Systems/FontSystem.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class IGame;
struct SEventContext;

class Application {
public:
	struct SConfig {
		short start_x;
		short start_y;
		short start_width;
		short start_height;

		const char* name = nullptr;

		FontSystemConfig FontConfig;
		std::vector<RenderViewConfig> Renderviews;
	};

public:
	DAPI Application() : GameInst(nullptr) {}
	DAPI Application(IGame* gameInstance) { GameInst = gameInstance; }
	~Application() {};

public:
	DAPI bool Initialize();
	DAPI bool Run();
	void GetFramebufferSize(unsigned int* width, unsigned int* height);

	// Event handlers
	bool OnEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context);
	bool OnResized(eEventCode code, void* sender, void* listener_instance, SEventContext context);

public:
	// Instance
	IGame* GameInst = nullptr;
	SPlatformState platform;

	// Run
	bool is_running;
	bool is_suspended;
	short width;
	short height;

	// Timer
	Clock AppClock;
	double last_time;

	bool Initialized = false;
};
