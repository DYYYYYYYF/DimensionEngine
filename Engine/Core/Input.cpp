#include "Input.hpp"
#include "Event.hpp"
#include "DMemory.hpp"
#include "EngineLogger.hpp"

namespace Core{
struct SKeyboardState {
	bool keys[256];
};

struct SMouseState {
	short x, y;
	bool buttons[Buttons::eButton_Max];
};

struct SInputState {
	SKeyboardState keyboard_current;
	SKeyboardState keyboard_previous;

	SMouseState mouse_current;
	SMouseState mouse_previous;
};

static bool Initialized = false;
static SInputState InputState = {};

void InputInitialize() {
	Memory::Zero(&InputState, sizeof(SInputState));
	Initialized = true;
	LOG_INFO("Input system initialized.");
}

void InputShutdown() {
	// TODO: Add shutdown routines when needed.
	Initialized = false;
}

void InputUpdate(double delta_time) {
	if (!Initialized) {
		return;
	}

	// Copy states
	Memory::Copy(&InputState.keyboard_previous, &InputState.keyboard_current, sizeof(SKeyboardState));
	Memory::Copy(&InputState.mouse_previous, &InputState.mouse_current, sizeof(SMouseState));
}

void InputProcessKey(Keys key, bool pressed) {
	if (InputState.keyboard_current.keys[key] != pressed) {
		InputState.keyboard_current.keys[key] = pressed;

		SEventContext context;
		context.data.u16[0] = key;
		EngineEvent::Fire(pressed ? eEventCode::eEvent_Code_Key_Pressed : eEventCode::eEvent_Code_Key_Released, 0, context);
	}
}

void InputProcessButton(Buttons button, bool pressed) {
	if (InputState.mouse_current.buttons[button] != pressed) {
		InputState.mouse_current.buttons[button] = pressed;

		SEventContext context;
		context.data.u16[0] = button;
		EngineEvent::Fire(pressed ? eEventCode::eEvent_Code_Button_Pressed : eEventCode::eEvent_Code_Button_Released, 0, context);
	}
}

void InputProcessMouseMove(short x, short y) {
	if (InputState.mouse_current.x != x || InputState.mouse_current.y != y) {
		// Update
		InputState.mouse_current.x = x;
		InputState.mouse_current.y = y;

		//Fire the event
		SEventContext context;
		context.data.i16[0] = x;
		context.data.i16[1] = y;
		EngineEvent::Fire(eEventCode::eEvent_Code_Mouse_Moved, 0, context);
	}
}

void InputProcessMouseWheel(char z_delta) {
	// NOTE: no internal state to update

	// Dispatch
	SEventContext context;
	context.data.u8[0] = z_delta;
	EngineEvent::Fire(eEventCode::eEvent_Code_Mouse_Wheel, 0, context);
}

bool InputIsKeyDown(Keys key) {
	if (!Initialized) {
		return false;
	}

	return InputState.keyboard_current.keys[key] == true;
}

bool InputIsKeyUp(Keys key) {
	if (!Initialized) {
		return false;
	}

	return InputState.keyboard_current.keys[key] == false;
}

bool InputWasKeyDown(Keys key) {
	if (!Initialized) {
		return false;
	}

	return InputState.keyboard_previous.keys[key] == true;
}

bool InputWasKeyUp(Keys key) {
	if (!Initialized) {
		return false;
	}

	return InputState.keyboard_previous.keys[key] == false;
}

bool InputeIsButtonDown(Buttons button) {
	if (!Initialized) {
		return false;
	}

	return InputState.mouse_current.buttons[button] == true;
}

bool InputIsButtonUp(Buttons button) {
	if (!Initialized) {
		return false;
	}

	return InputState.mouse_current.buttons[button] == false;
}

bool InputWasButtonDown(Buttons button) {
	if (!Initialized) {
		return false;
	}

	return InputState.mouse_previous.buttons[button] == true;
}

bool InputWasButtonUp(Buttons button) {
	if (!Initialized) {
		return false;
	}

	return InputState.mouse_previous.buttons[button] == false;
}

void InputGetMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = InputState.mouse_current.x;
	y = InputState.mouse_current.y;
}
void InputGetPreviousMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = InputState.mouse_previous.x;
	y = InputState.mouse_previous.y;
}


}	// namespace Input