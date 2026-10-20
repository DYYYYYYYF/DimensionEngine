#pragma once

#include "Containers/TArray.hpp"
#include "Defines.hpp"
#include <functional>
#include <string>
#include <vector>
#include <stack>
#include <Logger.hpp>

typedef std::function<bool(Log::Logger::Level, const std::string&)> PFN_ConsoleWrite;

struct CommandContext {
public:
	TArray<std::string> Arguments;
};
typedef std::function<void(CommandContext context)> PFN_ConsoleCommand;

class Console {
private:
	struct Consumer {
	public:
		PFN_ConsoleWrite Callback;
		void* Instance = nullptr;
	};

	struct Command {
		std::string Name;
		unsigned char ArgCount;
		PFN_ConsoleCommand Func;
	};

public:
	Console(){}
	virtual ~Console() {}

public:
	static void Initialize();
	static void Shutdown();

	static DAPI void RegisterConsumer(PFN_ConsoleWrite callback);
	static DAPI void UnregisterConsumer(PFN_ConsoleWrite callback);
	static DAPI void WriteLine(Log::Logger::Level level, const std::string& msg);
	static DAPI bool RegisterCommand(const std::string& cmd, unsigned char arg_count, PFN_ConsoleCommand func);
	static DAPI bool ExecuteCommand(const std::string& cmd);

private:
	static TArray<Consumer> RegisteredConsumers;
	static TArray<Command> RegisteredCommands;
};