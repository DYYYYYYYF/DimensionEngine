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
static SInputState state = {};

void InputInitialize() {
	Memory::Zero(&state, sizeof(SInputState));
	Initialized = true;
	UL_INFO("Input system initialized.");
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
	Memory::Copy(&state.keyboard_previous, &state.keyboard_current, sizeof(SKeyboardState));
	Memory::Copy(&state.mouse_previous, &state.mouse_current, sizeof(SMouseState));
}

void InputProcessKey(Keys key, bool pressed) {
	if (state.keyboard_current.keys[key] != pressed) {
		state.keyboard_current.keys[key] = pressed;

		SEventContext context;
		context.data.u16[0] = key;
		EventFire(pressed ? Core::eEvent_Code_Key_Pressed : Core::eEvent_Code_Key_Released, 0, context);
	}
}

void InputProcessButton(Buttons button, bool pressed) {
	if (state.mouse_current.buttons[button] != pressed) {
		state.mouse_current.buttons[button] = pressed;

		SEventContext context;
		context.data.u16[0] = button;
		EventFire(pressed ? Core::eEvent_Code_Button_Pressed : Core::eEvent_Code_Button_Released, 0, context);
	}
}

void InputProcessMouseMove(short x, short y) {
	if (state.mouse_current.x != x || state.mouse_current.y != y) {
		// TODO: Enable this if debugging
		// printf("Mouse position: %i, %d", x, y);

		// Update
		state.mouse_current.x = x;
		state.mouse_current.y = y;

		//Fire the event
		SEventContext context;
		context.data.u16[0] = x;
		context.data.u16[1] = y;
		Core::EventFire(Core::eEvent_Code_Mouse_Moved, 0, context);
	}
}

void InputProcessMouseWheel(char z_delta) {
	// NOTE: no internal state to update

	// Dispatch
	SEventContext context;
	context.data.u8[0] = z_delta;
	Core::EventFire(Core::eEvent_Code_Mouse_Wheel, 0, context);
}

bool InputIsKeyDown(Keys key) {
	if (Initialized) {
		return false;
	}

	return state.keyboard_current.keys[key] == true;
}

bool InputIsKeyUp(Keys key) {
	if (Initialized) {
		return false;
	}

	return state.keyboard_current.keys[key] == false;
}

bool InputWasKeyDown(Keys key) {
	if (Initialized) {
		return false;
	}

	return state.keyboard_previous.keys[key] == true;
}

bool InputWasKeyUp(Keys key) {
	if (Initialized) {
		return false;
	}

	return state.keyboard_previous.keys[key] == false;
}

bool InputeIsButtonDown(Buttons button) {
	if (Initialized) {
		return false;
	}

	return state.mouse_current.buttons[button] == true;
}

bool InputIsButtonUp(Buttons button) {
	if (Initialized) {
		return false;
	}

	return state.mouse_current.buttons[button] == false;
}

bool InputWasButtonDown(Buttons button) {
	if (Initialized) {
		return false;
	}

	return state.mouse_previous.buttons[button] == true;
}

bool InputWasButtonUp(Buttons button) {
	if (Initialized) {
		return false;
	}

	return state.mouse_previous.buttons[button] == false;
}

void InputGetMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = state.mouse_current.x;
	y = state.mouse_current.y;
}
void InputGetPreviousMousePosition(int& x, int& y) {
	if (!Initialized) {
		x = 0;
		y = 0;
		return;
	}

	x = state.mouse_previous.x;
	y = state.mouse_previous.y;
}


}	// namespace Input