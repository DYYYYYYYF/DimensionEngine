#pragma once
#include <Core/Keymap.hpp>

class IGame;

class Keybind {
public:
	void Setup(IGame* game);

public:
	// Global
	void GameOnEscape(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnYaw(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnPitch(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveForward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveBackward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveLeft(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveRight(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveUp(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnMoveDown(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnChangeConsoleVisibility(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnSetRenderModeDefault(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnSetRenderModeNormal(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnSetRenderModeBlinnphong(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnSetRenderModeDepth(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnPrintMemory(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnLoadScene(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnCompilerShader(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnTemplate(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);

	// Console
	void GameOnConsoleScroll(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);
	void GameOnConsoleScrollHold(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data);

};