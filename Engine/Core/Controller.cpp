#include "Controller.hpp"
#include "Event.hpp"
#include "DMemory.hpp"
#include "Keymap.hpp"
#include "EngineLogger.hpp"

bool Controller::Initialized = false;
Controller::SKeyboardState Controller::keyboard_current;
Controller::SKeyboardState Controller::keyboard_previous;

Controller::SMouseState Controller::mouse_current;
Controller::SMouseState Controller::mouse_previous;

std::vector<Keymap*> Controller::KeymapStack;

Controller::~Controller() {
	for (size_t i = 0; i < KeymapStack.size(); ++i) {
		if (KeymapStack[i] != nullptr) {
			DeleteObject(KeymapStack[i]);
		}
	}
}

void Controller::Initialize() {
	Initialized = true;
	GLOG(Log::eInfo, "Input system initialized.");
}

void Controller::Shutdown() {
	// TODO: Add shutdown routines when needed.
	Initialized = false;
}

void Controller::Update(double delta_time) {
	if (!Initialized) {
		return;
	}

	// Handle hold bindings.
	for (size_t i = 0; i < (size_t)eKeys::Max; ++i) {
		eKeys Key = eKeys(i);
		if (IsKeyDown(Key) && WasKeyDown(Key)) {
			int MapCount = (int)KeymapStack.size();
			for (int m = MapCount - 1; m >= 0; m--) {
				Keymap* Map = KeymapStack[m];
				if (Map == nullptr) continue;

				Keymap::Binding* Bind = &Map->Entries[i].Bindings[0];
				bool Unset = false;
				while (Bind) {
					// If an unset is detected, stop processing.
					if (Bind->Type == KeymapEntryBindType::eUnset) {
						Unset = true;
						break;
					}
					else if (Bind->Type == KeymapEntryBindType::eHold) {
						if (Bind->Callback && CheckModifiers(Bind->Modifiers)) {
							Bind->Callback(Key, Bind->Type, Bind->Modifiers, Bind->UserData);
						}
					}

					Bind = Bind->Next;
				}

				// If an unset is detected or the map is marked to override all, stop processing.
				if (Unset || Map->OverrideAll) {
					break;
				}
			}
		}
	}

	// Copy states
	Memory::Copy(&keyboard_previous, &keyboard_current, sizeof(SKeyboardState));
	Memory::Copy(&mouse_previous, &mouse_current, sizeof(SMouseState));
}

void Controller::ProcessKey(eKeys key, bool pressed) {
	if (keyboard_current.keys[(unsigned int)key] != pressed) {
		keyboard_current.keys[(unsigned int)key] = pressed;

		// Check key for key bindings.
		int MapCount = (int)KeymapStack.size();
		for (int m = MapCount - 1; m >= 0; m--) {
			Keymap* Map = KeymapStack[m];
			if (Map == nullptr) continue;

			Keymap::Binding* Bind = Map->Entries[(int)key].Bindings;
			bool Unset = false;
			while (Bind) {
				if (Bind->Type == KeymapEntryBindType::eUnset) {
					Unset = true;
					break;
				}
				else if (pressed && Bind->Type == KeymapEntryBindType::ePress) {
					if (Bind->Callback && CheckModifiers(Bind->Modifiers)) {
						Bind->Callback(key, Bind->Type, Bind->Modifiers, Bind->UserData);
					}
				}
				else if (!pressed && Bind->Type == KeymapEntryBindType::eRelease) {
					if (Bind->Callback && CheckModifiers(Bind->Modifiers)) {
						Bind->Callback(key, Bind->Type, Bind->Modifiers, Bind->UserData);
					}
				}

				Bind = Bind->Next;
			}

			if (Unset || Map->OverrideAll) {
				break;
			}
		}

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

void Controller::PushKeymap(Keymap* map) {
	KeymapStack.push_back(map);
}

void Controller::PopKeymap() {
	KeymapStack.erase(KeymapStack.end() - 1);
}

bool Controller::CheckModifiers(uint32_t modifiers) {
	if (modifiers & (uint32_t)KeymapModifierFlagBits::eShitf) {
		if (!IsKeyDown(eKeys::Shift) && !IsKeyDown(eKeys::LShift) && !IsKeyDown(eKeys::RShift)) {
			return false;
		}
	}
	if(modifiers & (uint32_t)KeymapModifierFlagBits::eControl) {
		if (!IsKeyDown(eKeys::Control) && !IsKeyDown(eKeys::LControl) && !IsKeyDown(eKeys::RControl)) {
			return false;
		}
	}
	if (modifiers & (uint32_t)KeymapModifierFlagBits::eAlt) {
		if (!IsKeyDown(eKeys::Alt)) {
			return false;
		}
	}

	return true;
}
