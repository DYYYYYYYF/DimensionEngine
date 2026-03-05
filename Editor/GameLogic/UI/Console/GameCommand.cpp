#include "GameCommand.h"
#include <Core/Event.hpp>
#include <Core/EngineLogger.hpp>

void GameCommand::GameExit(CommandContext cmd) {
	EngineEvent::Fire(eEventCode::Application_Quit, nullptr, SEventContext());
}

void GameCommand::GameOnCompilerShader(CommandContext cmd) {
    size_t ArgCount = cmd.Arguments.size();
    if (ArgCount > 0){
        std::string args = "";
        for (size_t i =0; i < ArgCount; ++i){
            args += cmd.Arguments[i];
        }
        
        GLOG(Log::eInfo, "Please complete 'GameOnCompilerShader' method. Arguments: %s.", args.c_str());
    } else {
        GLOG(Log::eInfo, "Please complete 'GameOnCompilerShader' method.");
    }
}

void GameCommand::Setup() {
	Console::RegisterCommand("exit", 0, std::bind(&GameCommand::GameExit, this, std::placeholders::_1));
	Console::RegisterCommand("quit", 0, std::bind(&GameCommand::GameExit, this, std::placeholders::_1));
    Console::RegisterCommand("compile shader", 1, std::bind(&GameCommand::GameOnCompilerShader, this, std::placeholders::_1));
}
