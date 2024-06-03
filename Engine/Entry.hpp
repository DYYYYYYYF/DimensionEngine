#include "Core/Application.hpp"
#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "GameType.hpp"

extern bool CreateGame(SGame* out_game);

int main(void) {

    if (!Memory::Initialize(MEBIBYTES(200))) {
        UL_ERROR("Failed to initialize memory system; shuting down.");
        return false;
    }

	SGame GameInstance;
	if (!CreateGame(&GameInstance)) {
		UL_FATAL("Could not Create Game!");
		return -1;
	}

    if (!GameInstance.render || !GameInstance.update || !GameInstance.initialize || !GameInstance.on_resize) {
        UL_FATAL("Game's function pointers must be assigned!");
        return -2;
    }



    if (!ApplicationCreate(&GameInstance)) {
        UL_INFO("Application create failed!");
        return 1;
    }

    if (!ApplicationRun()) {
        UL_INFO("Application did not shutdown gracefully!");
        return 2;
    }

    Memory::Shutdown();

    return 0;
}