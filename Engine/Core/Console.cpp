#include "Console.hpp"
#include "Core/Utils.hpp"
#include "Core/Event.hpp"

const uint32_t MAX_CONSUMER_COUNT = 10;

std::vector<Console::Consumer> Console::RegisteredConsumers;
std::vector<Console::Command> Console::RegisteredCommands;

void Console::Initialize() {

}

void Console::Shutdown() {

}

void Console::RegisterConsumer(PFN_ConsoleWrite callback) {
	Consumer NewConsumer;
	NewConsumer.Instance = nullptr;
	NewConsumer.Callback = std::move(callback);
	RegisteredConsumers.push_back(NewConsumer);
}

void Console::UnregisterConsumer(PFN_ConsoleWrite callback) {
	for (size_t i = 0; i < RegisteredConsumers.size(); ++i) {
		Consumer& consumer = RegisteredConsumers[i];
		if (consumer.Callback.target<bool(Log::Logger::Level, const std::string&)>() ==
			callback.target<bool(Log::Logger::Level, const std::string&)>()) {
			RegisteredConsumers.erase(RegisteredConsumers.begin() + i);
			continue;
		}
	}
}

void Console::WriteLine(Log::Logger::Level level, const std::string& msg) {
	// Notify each consumer that a line has been added.
	for (unsigned char i = 0; i < RegisteredConsumers.size(); ++i) {
		RegisteredConsumers[i].Callback(level, msg);
	}
}

bool Console::RegisterCommand(const std::string& cmd, unsigned char arg_count, PFN_ConsoleCommand func) {
	// Make sure it doesn't already exist.
	uint32_t CommandCount = (uint32_t)RegisteredCommands.size();
	for (uint32_t i = 0; i < CommandCount; ++i) {
		if (RegisteredCommands[i].Name.compare(cmd) == 0) {
			GLOG(Log::eError, "Command already registered: %s.", cmd.c_str());
			return false;
		}
	}

	Command NewCommand;
	NewCommand.ArgCount = arg_count;
	NewCommand.Func = func;
	NewCommand.Name = cmd;
	RegisteredCommands.push_back(NewCommand);

	return true;
}

bool Console::ExecuteCommand(const std::string& cmd) {
	if (cmd.length() == 0) {
		return false;
	}

	// TODO: If strings are ever used as arguments, this will split improperly.
	std::vector<std::string> Parts = Utils::StringSplit(cmd, '-', true, false);
	if (Parts.size() < 1){
		Parts.clear();
		std::vector<std::string>().swap(Parts);
		return false;
	}

	// Write the line back out to the console for reference.
	std::string Temp = "-->" + cmd;
	WriteLine(Log::Logger::eINFO, Temp);

	// Strings are slow. But it's a console. It doesn't need to be lightning fast.
	bool HasError = false;
	bool CommandFound = false;
	uint32_t CommandCount = (uint32_t)RegisteredCommands.size();
	for (uint32_t i = 0; i < CommandCount; ++i) {
		Console::Command* Cmd = &RegisteredCommands[i];
		if (Cmd->Name.compare(Parts[0]) == 0) {
			CommandFound = true;
			unsigned char ArgCount = (unsigned char)(Parts.size() - 1);
			if (Cmd->ArgCount != ArgCount) {
				GLOG(Log::eError, "The console command '%s' requires %u arguments but %u were provided.", Cmd->Name.c_str(), Cmd->ArgCount, ArgCount);
                // TODO: Should handle it inside callback, because it may has default param.
				// HasError = true;
			}
            
            // Execute
            CommandContext Context;
            if (ArgCount > 0) {
                Context.Arguments.resize(ArgCount);
                for (unsigned char j = 0; j < ArgCount; ++j) {
                    Context.Arguments[j] = Parts[j + 1];
                }
            }

            Cmd->Func(Context);
            if (!Context.Arguments.empty()) {
                Context.Arguments.clear();
                std::vector<std::string>().swap(Context.Arguments);
            }

			break;
		}
	}

	if (!CommandFound) {
		GLOG(Log::eError, "The command '%s' does not exist.", Parts[0].c_str());
		HasError = true;
	}

	if (!Parts.empty()) {
		Parts.clear();
		std::vector<std::string>().swap(Parts);
	}

	return !HasError;
}
