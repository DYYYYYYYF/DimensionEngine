#pragma once

#include "Containers/TStaticArray.hpp"
#include "Defines.hpp"
#include "Event.hpp"
#include "Controller.hpp"
#include <array>
#include <functional>

enum class KeymapModifierFlagBits {
	eNone = 0x0,
	eShitf = 0x1,
	eControl = 0x2,
	eAlt = 0x4
};

typedef uint32_t KeymapModifierFlags;

enum class KeymapEntryBindType {
	eUndefined = 0x0,
	ePress = 0x1,
	eRelease = 0x2,
	eHold = 0x4,
	eUnset = 0x8
};

typedef std::function<void(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data)> PFN_KeybindCallback;

class Keymap {
public:
	struct Binding {
		KeymapEntryBindType Type = KeymapEntryBindType::eUndefined;
		KeymapModifierFlags Modifiers = 0;
		PFN_KeybindCallback Callback = nullptr;
		void* UserData = nullptr;
		Keymap::Binding* Next = nullptr;
	};

	struct Entry
	{
		eKeys Key;
		// List
		Keymap::Binding* Bindings = nullptr;
	};

public:
	DAPI Keymap();
	DAPI virtual ~Keymap();

public:
	DAPI void AddBinding(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data, PFN_KeybindCallback callback);
	DAPI void RemoveBinding(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, PFN_KeybindCallback callback);

public:
	bool OverrideAll;
	TStaticArray<Keymap::Entry, (size_t)eKeys::Max> Entries;
};
