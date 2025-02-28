#pragma once

#include "Defines.hpp"
#include "Clock.hpp"
#include "Event.hpp"
#include "Resources/Skybox.hpp"
#include "Systems/FontSystem.hpp"
#include "Renderer/Interface/IRenderView.hpp"

class IGame;
struct SEventContext;
class Controller;

class Application {
public:
	struct SConfig {
		short start_x = 0;
		short start_y = 0;
		short start_width = 1920;
		short start_height = 1080;

		std::string name;

		FontSystemConfig FontConfig;
		std::vector<RenderViewConfig> Renderviews;
	};

public:
	DAPI Application() : GameInst(nullptr), GameController(nullptr), is_running(false), is_suspended(false),
		width(1920), height(1080), last_time(0.0), Initialized(false){}
	DAPI Application(IGame* gameInstance) : GameController(nullptr), is_running(false), is_suspended(false),
		width(1920), height(1080), last_time(0.0), Initialized(false) {
		GameInst = gameInstance;
	}
	virtual ~Application() {};

public:
	DAPI bool Initialize();
	DAPI bool Run();
	void GetFramebufferSize(unsigned int* width, unsigned int* height);

	// Event handlers
	bool OnEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context);
	bool OnResized(eEventCode code, void* sender, void* listener_instance, SEventContext context);

public:
	// Instance
	IGame* GameInst;
	SPlatformState platform;
	Controller* GameController;

	// Run
	bool is_running;
	bool is_suspended;
	short width;
	short height;

	// Timer
	Clock AppClock;
	double last_time;

	bool Initialized;
};
