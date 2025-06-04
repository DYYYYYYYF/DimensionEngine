#include "GameCommands.hpp"
#include <Core/Console.hpp>
#include <Core/Event.hpp>
#include <Core/EngineLogger.hpp>

void GameExit(CommandContext cmd) {
	EngineEvent::Fire(eEventCode::Application_Quit, nullptr, SEventContext());
}

void GameOnCompilerShader(CommandContext cmd) {
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
	
	// Reload
	// SEventContext Context = {};
	// EngineEvent::Fire(eEventCode::Reload_Shader_Module, nullptr, Context);
}

void GameCommand::Setup() {
	Console::RegisterCommand("exit", 0, &GameExit);
	Console::RegisterCommand("quit", 0, &GameExit);
    Console::RegisterCommand("compile shader", 1, &GameOnCompilerShader);
}
