#pragma once
#include <Core/Console.hpp>

class GameCommand {
public:
	void Setup();

public:
	void GameExit(CommandContext cmd);
	void GameOnCompilerShader(CommandContext cmd);
};