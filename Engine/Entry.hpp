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

	IGame* GameInst = new GameInstance();
	if (!CreateGame(GameInst)) {
		LOG_FATAL("Could not Create Game!");
		return -1;
	}

    if (!ApplicationCreate(GameInst)) {
        LOG_INFO("Application create failed!");
        return 1;
    }

    if (!ApplicationRun()) {
        LOG_INFO("Application did not shutdown gracefully!");
        return 2;
    }

    Memory::Shutdown();

    return 0;
}
