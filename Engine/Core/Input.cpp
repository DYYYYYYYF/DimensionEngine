#include "Input.hpp"
#include "Event.hpp"
#include "DMemory.hpp"
#include "EngineLogger.hpp"

bool Controller::Initialized = false;
Controller::SKeyboardState Controller::keyboard_current;
Controller::SKeyboardState Controller::keyboard_previous;

Controller::SMouseState Controller::mouse_current;
Controller::SMouseState Controller::mouse_previous;

void Controller::Initialize() {
	Initialized = true;
	LOG_INFO("Input system initialized.");
}

void Controller::Shutdown() {
	// TODO: Add shutdown routines when needed.
	Initialized = false;
}

void Controller::Update(double delta_time) {
	if (!Initialized) {
		return;
	}

	// Copy states
	Memory::Copy(&keyboard_previous, &keyboard_current, sizeof(SKeyboardState));
	Memory::Copy(&mouse_previous, &mouse_current, sizeof(SMouseState));
}

void Controller::ProcessKey(eKeys key, bool pressed) {
	if (keyboard_current.keys[(unsigned int)key] != pressed) {
		keyboard_current.keys[(unsigned int)key] = pressed;

		SEventContext context;
		context.data.u16[0] = (unsigned int)key;
		EngineEvent::Fire(pressed ? eEventCode::Key_Pressed : eEventCode::Key_Released, 0, context);
	}
}

void Controller::ProcessButton(eButtons button, bool pressed) {
	if (mouse_current.buttons[(unsigned int)button] != pressed) {
		mouse_current.buttons[(unsigned int)button] = pressed;

		SEventContext context;
		context.data.u16[0] = (unsigned int)button;
		EngineEvent::Fire(pressed ? eEventCode::Button_Pressed : eEventCode::Button_Released, 0, context);
	}
}

void Controller::ProcessMouseMove(short x, short y) {
	if (mouse_current.x != x || mouse_current.y != y) {
		// Update
		mouse_current.x = x;
		mouse_current.y = y;

		//Fire the event
		SEventContext context;
		context.data.i16[0] = x;
		context.data.i16[1] = y;
		EngineEvent::Fire(eEventCode::Mouse_Moved, 0, context);
	}
}

void Controller::ProcessMouseWheel(char z_delta) {
	// NOTE: no internal state to update

	// Dispatch
	SEventContext context;
	context.data.u8[0] = z_delta;
	EngineEvent::Fire(eEventCode::Mouse_Wheel, 0, context);
}

bool Controller::IsKeyDown(eKeys key) {
	if (!Initialized) {
		return false;
	}

	return keyboard_current.keys[(unsigned int)key] == true;
}

bool Controller::IsKeyUp(eKeys key) {
	if (!Initialized) {
		return false;
	}

	return keyboard_current.keys[(unsigned int)key] == false;
}

bool Controller::WasKeyDown(eKeys key) {
	if (!Initialized) {
		return false;
	}

	return keyboard_previous.keys[(unsigned int)key] == true;
}

bool Controller::WasKeyUp(eKeys key) {
	if (!Initialized) {
		return false;
	}

	return keyboard_previous.keys[(unsigned int)key] == false;
}

bool Controller::IsButtonDown(eButtons button) {
	if (!Initialized) {
		return false;
	}

	return mouse_current.buttons[(unsigned int)button] == true;
}

bool Controller::IsButtonUp(eButtons button) {
	if (!Initialized) {
		return false;
	}

	return mouse_current.buttons[(unsigned int)button] == false;
}

bool Controller::WasButtonDown(eButtons button) {
	if (!Initialized) {
		return false;
	}

	return mouse_previous.buttons[(unsigned int)button] == true;
}

bool Controller::WasButtonUp(eButtons button) {
	if (!Initialized) {
		return false;
	}

	return mouse_previous.buttons[(unsigned int)button] == false;
}

void Controller::GetMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = mouse_current.x;
	y = mouse_current.y;
}
void Controller::GetPreviousMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = mouse_previous.x;
	y = mouse_previous.y;
}
