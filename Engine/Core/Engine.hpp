#pragma once

#include "Defines.hpp"
#include "Clock.hpp"
#include "Event.hpp"
#include "Systems/FontSystem.hpp"
#include "Rendering/Interface/IRenderView.hpp"

class IGame;
struct SEventContext;
class Controller;

class Engine {
public:
	DAPI Engine() : GameInst(nullptr), GameController(nullptr), is_running(false), is_suspended(false),
		width(1920), height(1080), last_time(0.0), Initialized(false){}
	DAPI Engine(IGame* gameInstance) : GameController(nullptr), is_running(false), is_suspended(false),
		width(1920), height(1080), last_time(0.0), Initialized(false) {
		GameInst = gameInstance;
	}
	virtual ~Engine() {};

public:
	DAPI bool Initialize();
	DAPI bool Run();
	void GetFramebufferSize(unsigned int* width, unsigned int* height);

	// Event handlers
	bool OnEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context);
	bool OnResized(eEventCode code, void* sender, void* listener_instance, SEventContext context);

public:
	IRenderer* Renderer;

	// Instance
	IGame* GameInst;
	SPlatformState platform;
	Controller* GameController;

	// Run
	bool is_running;
	bool is_suspended;
	uint16_t width;
	uint16_t height;

	// Timer
	Clock AppClock;
	double last_time;

	bool Initialized;
};
