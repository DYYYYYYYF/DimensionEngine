#include "Keybinds.hpp"
#include "Game.hpp"
#include <Core/Keymap.hpp>
#include <Core/DMemory.hpp>
#include <Core/EngineLogger.hpp>
#include <Renderer/Camera.hpp>

void GameOnEscape(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	EngineEvent::Fire(eEventCode::Application_Quit, nullptr, SEventContext());
}

void GameOnYaw(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float f = 0.0f;
	if (key == eKeys::Left || key == eKeys::A) {
		f = 1.0f;
	}
	else if (key == eKeys::Right || key == eKeys::D) {
		f = -1.0f;
	}
	
	GameInst->WorldCamera->RotateYaw(f * GameInst->DeltaTime);
}

void GameOnPitch(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float f = 0.0f;
	if (key == eKeys::Up) {
		f = 1.0f;
	}
	else if (key == eKeys::Down) {
		f = -1.0f;
	}

	GameInst->WorldCamera->RotatePitch(f * GameInst->DeltaTime);
}

void GameOnMoveForward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveForward(TempSpeed * GameInst->DeltaTime);
}

void GameOnMoveBackward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveBackward(TempSpeed * GameInst->DeltaTime);
}

void GameOnMoveLeft(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveLeft(TempSpeed * GameInst->DeltaTime);
}

void GameOnMoveRight(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveRight(TempSpeed * GameInst->DeltaTime);
}

void GameOnMoveUp(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveUp(TempSpeed * GameInst->DeltaTime);
}

void GameOnMoveDown(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	float TempSpeed = 50.0f;
	if (Controller::IsKeyDown(eKeys::Shift)) {
		TempSpeed *= 2.0f;
	}
	GameInst->WorldCamera->MoveDown(TempSpeed * GameInst->DeltaTime);
}

void GameOnChangeConsoleVisibility(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	DebugConsoleActor* UsedConsole = GameInst->GameConsole;
	if (UsedConsole) {
		bool ConsoleVisible = GameInst->GameConsole->IsVisible();
		ConsoleVisible = !ConsoleVisible;
		UsedConsole->SetVisible(ConsoleVisible);

		if (ConsoleVisible) {
			Controller::PushKeymap(GameInst->ConsoleKeymap);
		}
		else {
			Controller::PopKeymap();
		}
	}
}

void GameOnSetRenderModeDefault(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Default;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void GameOnSetRenderModeNormal(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Normals;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void GameOnSetRenderModeBlinnphong(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Lighting;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void GameOnSetRenderModeDepth(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Depth;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void GameOnPrintMemory(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	static size_t AllocCount = 0;
	size_t PrevAllocCount = AllocCount;
	AllocCount = Memory::GetAllocateCount();
	size_t UsedMemory = AllocCount - PrevAllocCount;
	char* Usage = Memory::GetMemoryUsageStr();
	GLOG(Log::eInfo, Usage);

	size_t Size = 0;
	unsigned short  Alignment = 0;
	if (Memory::GetAlignmentSize(Usage, &Size, &Alignment)) {
		Memory::FreeAligned(Usage, Size, Alignment, MemoryType::eMemory_Type_String);
	}
	GLOG(Log::eDebug, "Allocations: %llu (%llu this frame)", AllocCount, UsedMemory);
}

void GameOnLoadScene(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	if (key == eKeys::O) {
		EngineEvent::Fire(eEventCode::Debug_0, user_data, SEventContext());
	}
	else if (key == eKeys::P) {
		EngineEvent::Fire(eEventCode::Debug_1, user_data, SEventContext());
	}
	else if (key == eKeys::L) {
		EngineEvent::Fire(eEventCode::Debug_2, user_data, SEventContext());
	}
	else if (key == eKeys::K) {
		EngineEvent::Fire(eEventCode::Debug_3, user_data, SEventContext());
	}
}

void GameOnCompilerShader(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		return;
	}

	//GameInst->TestPython.ExecuteFunc("CompileShaders", "glsl");
	// Reload
	SEventContext Context = {};
	const char* ShaderName = "Shader.Builtin.World";
	Memory::Copy(Context.data.c, ShaderName, strlen(ShaderName) +1);
	EngineEvent::Fire(eEventCode::Reload_Shader_Module, GameInst, Context);
}

void GameOnTemplate(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {}

// ------------------------------- Console ---------------------------------- //
void GameOnConsoleScroll(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	DebugConsoleActor* Console = (DebugConsoleActor*)user_data;
	if (Console) {
		if (key == eKeys::Up) {
			Console->MoveUp();
		}
		else if (key == eKeys::Down) {
			Console->MoveDown();
		}
	}
}

void GameOnConsoleScrollHold(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	GameInstance* GameInst = (GameInstance*)user_data;
	if (GameInst == nullptr) {
		GLOG(Log::eError, "Keybind::Setup() Invalid game pointer.");
		return;
	}

	static float AccumulatedTime = 0.0f;
	AccumulatedTime += GameInst->DeltaTime;
	if (AccumulatedTime >= 0.1f) {
		if (key == eKeys::Up) {
			GameInst->GameConsole->MoveUp();
		}
		else if (key == eKeys::Down) {
			GameInst->GameConsole->MoveDown();
		}
		AccumulatedTime = 0.0f;
	}
}

// -------------------------------- Setup ----------------------------------- //
void Keybind::Setup(IGame* game) {
	GameInstance* GameInst = (GameInstance*)game;
	if (GameInst == nullptr) {
		GLOG(Log::eError, "Keybind::Setup() Invalid game pointer.");
		return;
	}

	// Global keymap
	Keymap* GlobalKeymap = NewObject<Keymap>();
	GlobalKeymap->AddBinding(eKeys::Escape, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnEscape, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	Controller::PushKeymap(GlobalKeymap);

	// Game keymap
	Keymap* GameKeymap = NewObject<Keymap>();
	// Left
	GameKeymap->AddBinding(eKeys::A, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveLeft, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::Left, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnYaw, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Right
	GameKeymap->AddBinding(eKeys::D, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveRight, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::Right, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnYaw, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Up
	GameKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnPitch, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Down
	GameKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnPitch, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Console visible
	GameKeymap->AddBinding(eKeys::Grave, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnChangeConsoleVisibility, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Forward
	GameKeymap->AddBinding(eKeys::W, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveForward, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Backward
	GameKeymap->AddBinding(eKeys::S, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveBackward, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Up
	GameKeymap->AddBinding(eKeys::E, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveUp, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Down
	GameKeymap->AddBinding(eKeys::Q, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnMoveDown, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Renderview mode
	GameKeymap->AddBinding(eKeys::F1, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnSetRenderModeDefault, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F2, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnSetRenderModeBlinnphong, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F3, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnSetRenderModeNormal, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F4, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnSetRenderModeDepth, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Load models
	GameKeymap->AddBinding(eKeys::O, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnLoadScene, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::P, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnLoadScene, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::K, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnLoadScene, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::L, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnLoadScene, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Print memory
	GameKeymap->AddBinding(eKeys::M, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnPrintMemory, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Compiler shader
	GameKeymap->AddBinding(eKeys::G, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnCompilerShader, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	Controller::PushKeymap(GameKeymap);

	// Console keymap
	if (GameInst->ConsoleKeymap) {
		DeleteObject(GameInst->ConsoleKeymap);
		GameInst->ConsoleKeymap = nullptr;
	}

	GameInst->ConsoleKeymap = NewObject<Keymap>();
	GameInst->ConsoleKeymap->OverrideAll = true;
	GameInst->ConsoleKeymap->AddBinding(eKeys::Grave, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnChangeConsoleVisibility, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Escape, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnChangeConsoleVisibility, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	GameInst->ConsoleKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst->GameConsole,
		std::bind(&GameOnConsoleScroll, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst->GameConsole,
		std::bind(&GameOnConsoleScroll, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnConsoleScrollHold, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&GameOnConsoleScrollHold, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}