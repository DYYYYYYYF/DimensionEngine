#include "Keybinds.hpp"
#include "Game.hpp"
#include <Core/DMemory.hpp>
#include <Core/EngineLogger.hpp>
#include <Framework/Classes/CameraActor.h>

void Keybind::GameOnEscape(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)user_data;
	(void)type;
	(void)key;

	EngineEvent::Fire(eEventCode::Application_Quit, nullptr, SEventContext());
}

void Keybind::GameOnYaw(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;

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

void Keybind::GameOnPitch(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;

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

void Keybind::GameOnMoveForward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnMoveBackward(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnMoveLeft(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnMoveRight(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnMoveUp(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnMoveDown(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnChangeConsoleVisibility(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnSetRenderModeDefault(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;

	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Default;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void Keybind::GameOnSetRenderModeNormal(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;

	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Normals;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void Keybind::GameOnSetRenderModeBlinnphong(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;

	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Lighting;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void Keybind::GameOnSetRenderModeDepth(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;

	SEventContext Context;
	Context.data.i32[0] = ShaderRenderMode::eShader_Render_Mode_Depth;
	EngineEvent::Fire(eEventCode::Set_Render_Mode, nullptr, Context);
}

void Keybind::GameOnPrintMemory(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;

	static size_t AllocCount = 0;
	size_t PrevAllocCount = AllocCount;
	AllocCount = Memory::GetAllocateCount();
	size_t UsedMemory = AllocCount - PrevAllocCount;
	char* Usage = Memory::GetMemoryUsageStr();
	GLOG(Log::eInfo, Usage);

	size_t Size = 0;
	if (Memory::GetAlignmentSize(Usage, &Size, nullptr)) {
		Memory::FreeAligned(Usage, Size, MemoryType::eMemory_Type_String);
	}
	GLOG(Log::eDebug, "Allocations: %llu (%llu this frame)", AllocCount, UsedMemory);
}

void Keybind::GameOnLoadScene(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;

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

void Keybind::GameOnCompilerShader(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)key;

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

void Keybind::GameOnTemplate(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;
	(void)user_data;
	(void)key;
}

// ------------------------------- Console ---------------------------------- //
void Keybind::GameOnConsoleScroll(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;

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

void Keybind::GameOnConsoleScrollHold(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data) {
	(void)modifiers;
	(void)type;

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
		std::bind(&Keybind::GameOnEscape, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	Controller::PushKeymap(GlobalKeymap);

	// Game keymap
	Keymap* GameKeymap = NewObject<Keymap>();
	// Left
	GameKeymap->AddBinding(eKeys::A, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveLeft, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::Left, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnYaw, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Right
	GameKeymap->AddBinding(eKeys::D, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveRight, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::Right, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnYaw, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Up
	GameKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnPitch, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Down
	GameKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnPitch, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Console visible
	GameKeymap->AddBinding(eKeys::Grave, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnChangeConsoleVisibility, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Forward
	GameKeymap->AddBinding(eKeys::W, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveForward, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Backward
	GameKeymap->AddBinding(eKeys::S, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveBackward, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Up
	GameKeymap->AddBinding(eKeys::E, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveUp, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Down
	GameKeymap->AddBinding(eKeys::Q, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnMoveDown, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Renderview mode
	GameKeymap->AddBinding(eKeys::F1, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnSetRenderModeDefault, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F2, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnSetRenderModeBlinnphong, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F3, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnSetRenderModeNormal, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::F4, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnSetRenderModeDepth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Load models
	GameKeymap->AddBinding(eKeys::O, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnLoadScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::P, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnLoadScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::K, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnLoadScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameKeymap->AddBinding(eKeys::L, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnLoadScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// Print memory
	GameKeymap->AddBinding(eKeys::M, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnPrintMemory, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	// Compiler shader
	GameKeymap->AddBinding(eKeys::G, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnCompilerShader, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	Controller::PushKeymap(GameKeymap);

	// Console keymap
	if (GameInst->ConsoleKeymap) {
		DeleteObject(GameInst->ConsoleKeymap);
		GameInst->ConsoleKeymap = nullptr;
	}

	GameInst->ConsoleKeymap = NewObject<Keymap>();
	GameInst->ConsoleKeymap->OverrideAll = true;
	GameInst->ConsoleKeymap->AddBinding(eKeys::Grave, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnChangeConsoleVisibility, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Escape, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnChangeConsoleVisibility, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	GameInst->ConsoleKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst->GameConsole,
		std::bind(&Keybind::GameOnConsoleScroll, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::ePress, (uint32_t)KeymapModifierFlagBits::eNone, GameInst->GameConsole,
		std::bind(&Keybind::GameOnConsoleScroll, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Up, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnConsoleScrollHold, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	GameInst->ConsoleKeymap->AddBinding(eKeys::Down, KeymapEntryBindType::eHold, (uint32_t)KeymapModifierFlagBits::eNone, GameInst,
		std::bind(&Keybind::GameOnConsoleScrollHold, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}