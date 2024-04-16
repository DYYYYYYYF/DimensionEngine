#include "Event.hpp"
#include "DMemory.hpp"
#include "../Containers/TArray.hpp"

namespace Event {
	struct SRegisterEvent {
		void* listener;
		PFN_on_event callback;
	};

	struct SEventCodeEntry {
		TArray<SRegisterEvent> events;
	};

	// This should be more than enough coeds
#define MAX_MESSAGE_CODES 16384

	struct EventSystemState {
		TArray<SEventCodeEntry> registered;
	};

	// Event system internal state
	static bool IsInitialized = false;
	static EventSystemState state;

}

bool Event::EventInitialize() {
	if (IsInitialized == true) {
		return false;
	}

	IsInitialized = false;
	Memory::Zero(&state, sizeof(state));

	state.registered = TArray<SEventCodeEntry>(MAX_MESSAGE_CODES);

	IsInitialized = true;
	return IsInitialized;
}

void Event::EventShutdown() {
	for (unsigned short i = 0; i < MAX_MESSAGE_CODES; ++i) {
		if (!state.registered.IsEmpty()) {
			state.registered.Clear();
		}
	}
}

bool Event::EventRegister(unsigned short code, void* listener, PFN_on_event on_event) {
	if (IsInitialized == false) {
		return false;
	}

	if (state.registered.Data() == nullptr) {
		state.registered = TArray<SEventCodeEntry>(MAX_MESSAGE_CODES);
	}

	if (state.registered[code]->events.Data() == nullptr) {
		state.registered[code]->events = TArray<SRegisterEvent>();
	}

	size_t RegisterCount = state.registered[code]->events.Size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		if (state.registered[code]->events[i]->listener == listener) {
			// TODO: Warn
			return false;
		}
	}

	// If at this point, no duplication was found. proceed with registration
	SRegisterEvent NewEvent;
	NewEvent.listener = listener;
	NewEvent.callback = on_event;

	SEventCodeEntry* Entry = state.registered[code];
	Entry->events.Push(NewEvent);

	return true;
}

bool Event::EventUnregister(unsigned short code, void* listener, PFN_on_event on_event) {
	if (IsInitialized == false) {
		return false;
	}

	if (state.registered[code]->events.Data() == nullptr) {
		// TODO: Warn
		return false;
	}

	size_t RegisterCount = state.registered[code]->events.Size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent* event = state.registered[code]->events[i];
		if (event->listener == listener && event->callback == on_event) {
			state.registered.PopAt(i);
			return true;
		}
	}

	return false;
}

bool Event::EventFire(unsigned short code, void* sender, SEventContext context) {
	if (IsInitialized == false) {
		return false;
	}

	if (state.registered[code]->events.Data() == nullptr) {
		// TODO: Warn
		return false;
	}

	size_t RegisterCount = state.registered[code]->events.Size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent* event = state.registered[code]->events[i];
		if (event->callback(code, sender, event->listener, context)) {
			// Message has been handled, do not send to other listeners.
			return true;
		}
	}

	return false;
}
