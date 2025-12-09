#include "Keymap.hpp"
#include "DMemory.hpp"

Keymap::Keymap() {
	OverrideAll = false;
	for (size_t i = 0; i < Entries.size(); ++i) {
		Entries[i].Bindings = nullptr;
		Entries[i].Key = eKeys(i);
	}
}

Keymap::~Keymap() {

}

void Keymap::AddBinding(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data, PFN_KeybindCallback callback) {
	Entry* Ety = &Entries[(size_t)key];
	Binding* Node = Ety->Bindings;
	Binding* Previous = Ety->Bindings;
	while (Node) {
		Previous = Node;
		Node = Node->Next;
	}

	Binding* NewBinding = NewObject<Keymap::Binding>();
	NewBinding->Callback = std::move(callback);
	NewBinding->Modifiers = modifiers;
	NewBinding->Type = type;
	NewBinding->UserData = user_data;
	NewBinding->Next = nullptr;

	if (Previous) {
		Previous->Next = NewBinding;
	}
	else {
		Ety->Bindings = NewBinding;
	}
}

void Keymap::RemoveBinding(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, PFN_KeybindCallback callback) {
	(void)modifiers;
	(void)type;

	Entry* Ety = &Entries[(size_t)key];
	Binding* Node = Ety->Bindings;
	Binding* Previous = Ety->Bindings;

	while (Node) {
		if (Node->Callback.target<void(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data)>() ==
			callback.target<void(eKeys key, KeymapEntryBindType type, KeymapModifierFlags modifiers, void* user_data)>()) {
			// Remove
			Previous->Next = Node->Next;
			DeleteObject(Node);
			Node = nullptr;
			return;
		}

		Previous = Node;
		Node = Node->Next;
	}
}