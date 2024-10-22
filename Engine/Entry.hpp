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

    IGame* GameInst = NewObject<GameInstance>();
    if (GameInst == nullptr) {
		LOG_FATAL("Could not allocate memory for Game!");
		return -2;
    }

	if (!CreateGame(GameInst)) {
		LOG_FATAL("Could not Create Game!");
		return -1;
	}

    Application* App = NewObject<Application>(GameInst);
	if (App == nullptr) {
		LOG_FATAL("Could not allocate memory for Application!");
		return 0;
	}

    if (!App->Initialize()) {
		LOG_INFO("Application did not initialize gracefully!");
		return 1;
    }

    if (!App->Run()) {
        LOG_INFO("Application did not shutdown gracefully!");
        return 2;
    }

    if (App) {
        DeleteObject(App);
    }

    if (GameInst) {
        DeleteObject(GameInst);
    }

    Memory::Shutdown();

    return 0;
}
