#include "Core/Application.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "GameType.hpp"

// Init logger
static EngineLogger* GlobalLogger = new EngineLogger();

extern bool CreateGame(SGame* out_game);

int main(void) {

    if (!Memory::Initialize(MEBIBYTES(500))) {
        LOG_ERROR("Failed to initialize memory system; shuting down.");
        return 0;
    }

	SGame GameInstance;
	if (!CreateGame(&GameInstance)) {
		LOG_FATAL("Could not Create Game!");
		return -1;
	}

    if (!GameInstance.render || !GameInstance.update || !GameInstance.initialize || !GameInstance.on_resize) {
        LOG_FATAL("Game's function pointers must be assigned!");
        return -2;
    }



    if (!ApplicationCreate(&GameInstance)) {
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
