#include "Core/Engine.hpp"
#include "IGame.hpp"

// Init logger
static EngineLogger* GlobalLogger = new EngineLogger();

extern bool CreateGame(IGame* out_game);

int main(void) {

    if (!Memory::Initialize(MEBIBYTES(500))) {
        GLOG(Log::eError, "Failed to initialize memory system; shuting down.");
        return 0;
    }

    IGame* GameInst = NewObject<GameInstance>();
    if (GameInst == nullptr) {
		GLOG(Log::eFatal, "Could not allocate memory for Game!");
		return -2;
    }

	if (!CreateGame(GameInst)) {
		GLOG(Log::eFatal, "Could not Create Game!");
		return -1;
	}

    Engine* CoreEngine = NewObject<Engine>(GameInst);
	if (CoreEngine == nullptr) {
		GLOG(Log::eFatal, "Could not allocate memory for Application!");
		return 0;
	}

    if (!CoreEngine->Initialize()) {
		GLOG(Log::eInfo, "Application did not initialize gracefully!");
		return 1;
    }

    if (!CoreEngine->Run()) {
        GLOG(Log::eInfo, "Application did not shutdown gracefully!");
        return 2;
    }

    if (CoreEngine) {
        DeleteObject(CoreEngine);
    }

    if (GameInst) {
        DeleteObject(GameInst);
    }

    Memory::Shutdown();
    delete(GlobalLogger);

    return 0;
}
