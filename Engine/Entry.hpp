#include "Core/Application.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "GameType.hpp"

// Init logger
static EngineLogger* GlobalLogger = new EngineLogger();

extern bool CreateGame(IGame* out_game);

int main(void) {

    if (!Memory::Initialize(MEBIBYTES(500))) {
        LOG_ERROR("Failed to initialize memory system; shuting down.");
        return 0;
    }

	//IGame* GameInst = new GameInstance();
	IGame* GameInst = (IGame*)Memory::Allocate(sizeof(GameInstance), eMemory_Type_Game);
    if (GameInst == nullptr) {
		LOG_FATAL("Could not allocate memory for Game!");
		return -2;
    }
    GameInst = new (GameInst)GameInstance();
    ASSERT(GameInst);

	if (!CreateGame(GameInst)) {
		LOG_FATAL("Could not Create Game!");
		return -1;
	}

	Application* App = (Application*)Memory::Allocate(sizeof(Application), eMemory_Type_Application);
	if (App == nullptr) {
		LOG_FATAL("Could not allocate memory for Application!");
		return 0;
	}
    App = new (App)Application(GameInst);
    ASSERT(App);

    if (!App->Initialize()) {
		LOG_INFO("Application did not initialize gracefully!");
		return 1;
    }

    if (!App->Run()) {
        LOG_INFO("Application did not shutdown gracefully!");
        return 2;
    }

    if (App) {
        Memory::Free(App, sizeof(Application), eMemory_Type_Application);
    }

    if (GameInst) {
        Memory::Free(GameInst, sizeof(GameInstance), eMemory_Type_Game);
    }

    Memory::Shutdown();

    return 0;
}
